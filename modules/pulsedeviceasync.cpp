#ifdef PULSEASYNC

#include "pulsedeviceasync.h"
#include "audioformat.h"

#include <pulse/pulseaudio.h>
#include <pulse/error.h>
#include <pulse/stream.h>
#include <pulse/version.h>

#include <QMap>

/* https://i0.wp.com/i69.photobucket.com/albums/i68/Dragodon/KeyboardFaceSmash.gif << me coding this...
 * Ordre normal des procédures:
 * QIODevice *dev = new PulseDeviceASync(format,QString(),this);
 * inutile de vérifier le open dans la classe parente: la fonction retournera toujours true du moment que l'on ouvre pas la classe en readWrite.
 * en cas de pépin le device se close tout seul (j'ai fait ce choix pas ne pas rendre le 'open' blocant et ainsi perdre tout l'intéret de l'asynchrone
 *
 * dev->open(QIODevice::WriteOnly);
 *    makeContext
 *        starting the threaded mainloop
 *    makeStream (en cas de success du context (via le callback)
 *    makeChannelsMap
 *    emit(readyWrite);
 * dev->write(char*,int);
 * ...
 * dev->close;
 * destructeur.
 *
 * pour la lecture il n'est pas recomandé d'appeler readData sans avoir préalablement recu un signal readyRead()
 */

PulseDeviceASync::PulseDeviceASync(AudioFormat *format,const QString serverHost,QObject *parent) :
    QIODevice(parent)
{
    say("init start");
    this->format = format;
    this->serverHost = serverHost;
    //intialisating pointers to NULL value
    mainloop = NULL;
    mainloop_api = NULL;
    stream = NULL;
    context = NULL;
    readBuffer = NULL;

    say("setting sample specs");
    ss.channels = format->getChannelsCount();

    say("setting format");
    ss.format = this->getSampleFormat();

    say("setting sample rate");
    ss.rate = format->getSampleRate();
    if (!this->getDefaultSamplesRates().contains(ss.rate)) {
        say("WARNING: the requested sample rate is not in the default sample rates list: " + QString::number(ss.rate));
    }

    say("init ok");
}
PulseDeviceASync::~PulseDeviceASync() {

   if (stream) {
       say("deleting stream");
       pa_stream_drain(stream,NULL,NULL);
       pa_stream_disconnect(stream);
       pa_stream_drop(stream);
       stream = NULL;
   }
   if (context) {
       say("deleting context");
       pa_context_drain(context,NULL,NULL);
       pa_context_disconnect(context);
       context = NULL;
   }
   if (mainloop) {
       say("deleting mainloop");
       pa_threaded_mainloop_stop(mainloop);
       //pa_threaded_mainloop_unlock(mainloop);
       pa_threaded_mainloop_free(mainloop);
       mainloop = NULL;
   }
   if (readBuffer) {
       say("deleting buffer");
       readBuffer->disconnect();
       delete(readBuffer);
   }
   say("deleted");
}

//cette fonction est crade, mais je n'ai pas le choix et je commence à en avoir marre de me faire téraformer le trou du cul par l'api bordelique de pulseaudio !
//futur moi: pardonne moi.
void PulseDeviceASync::getSourcesDevices_cb(pa_context *c,const pa_source_info *i,int eol,void *userdata) {
    if (!c) return;
    (void) eol;
    QList<pa_source_info> *sources = (QList<pa_source_info>*) userdata;
    sources->append(*i);
}
void PulseDeviceASync::getSinkDevices_cb(pa_context *c,const pa_sink_info* i,int eol,void *userdata) {
    if (!c) return;
    (void) eol;
    QList<pa_sink_info> *sinks = (QList<pa_sink_info>*) userdata;
    sinks->append(*i);
}

QStringList PulseDeviceASync::getDevicesNames(QIODevice::OpenMode mode) {
    if (!this->isOpen()) {
        say("cannot retrive avaialbles devices: not connected to pulseaudio: use open().");
        return QStringList();
    }
    QStringList devicesNames;
    if (!context) return devicesNames;
    else if (mode == QIODevice::WriteOnly) {
        QList<pa_sink_info> sinks = getSinkDevices();
        say("devices found: " + QString::number(sinks.count()));
        const int m = sinks.count();
        for (int p = 0;p < m;p++) {
            devicesNames << sinks.at(p).name;
        }
    }
    else if (mode == QIODevice::ReadOnly) {
        QList<pa_source_info> sources = getSourcesDevices();
        const int m = sources.count();
        for (int p = 0;p < m;p++) devicesNames << sources.at(p).name;
    }
    return devicesNames;
}
QList<pa_source_info> PulseDeviceASync::getSourcesDevices() {
    QList<pa_source_info> sources;
    if (!context) return sources;
    pa_context_get_source_info_list(context,getSourcesDevices_cb,&sources);
    return sources;
}
QList<pa_sink_info> PulseDeviceASync::getSinkDevices() {
    QList<pa_sink_info> sinks;
    if (!context) return sinks;
    pa_context_get_sink_info_list(context,getSinkDevices_cb,&sinks);
    return sinks;
}
//ne surtout pas relire cette fonction: elle initialise tout un merdier pensé par les codeurs tordus de la lib de pulseaudio, *Dont touch: it's magic
//TGCM / *i have no idea of what i'm doing....
bool PulseDeviceASync::makeContext() {
    mainloop = pa_threaded_mainloop_new();

    if (!mainloop) return false;

    say("main loop: ok");

    mainloop_api = pa_threaded_mainloop_get_api(mainloop);

    if (!mainloop_api) return false;
    say("main loop api: ok");

    //context = pa_context_new_with_proplist(mainloop_api,this->name.toStdString().c_str(),NULL);
    context = pa_context_new(mainloop_api,this->objectName().toLocal8Bit().constData());

    say("requesting context to connect to pulseaudio server");

    char* host;
    if (serverHost.isEmpty()) {
        host = NULL;
        say("using default pulse server");
    }
    else host = serverHost.toLocal8Bit().data();

    say("server target: " + serverHost);

    pa_spawn_api api;
    api.atfork = NULL;
    api.postfork = NULL;
    api.prefork = NULL;
    say("connecting context to state callback");
    pa_context_set_state_callback(context,
                                  PulseDeviceASync::context_state_callback,
                                  this);

    say("connecting context");

    int error = pa_context_connect(context,
                                   (const char*) host,
                                   PA_CONTEXT_NOAUTOSPAWN,
                                   &api);


    if (error < 0) {
        say("unable to connect the context to server");
        say("error is: " + QString(pa_strerror(error)));
        return false;
    }
    say("context ok");
    pa_threaded_mainloop_start(mainloop);

    return true;
}
bool PulseDeviceASync::makeStream() {
    //cette fonction créé le flux: elle DOIT être appelée par le callback du context dans le cas ou celui à réusi sa connection à pulseaudio
    //elle est statique

    if (!makeChannelMap(&map)) {
        say("unable to get a channel map");
        return false;
    }

    stream = pa_stream_new(context,
                           this->objectName().toLocal8Bit().constData(),
                           &ss,
                           &map); //the channel map: NULL for default
    if (!stream) {
        say("unable to get a stream playback, the most common cause is that you ask a too high sample size, try with a lower (like 16)");
        return false;
    }
    say("connecting stream callback");
    pa_stream_set_state_callback(stream,PulseDeviceASync::stream_state_callback,this);

    int error = 0;

    if (this->openMode() == QIODevice::WriteOnly) {
        say("connecting playback");

        //pour les stream flags, de la doc est dispo ici:
        // http://freedesktop.org/software/pulseaudio/doxygen/def_8h.html#a6966d809483170bc6d2e6c16188850fc
        error = pa_stream_connect_playback(stream,  //stream pointer (pa_stream*)
                                   NULL, //name of the sink (NULL to default)
                                   NULL,                              //Buffering attributes, or NULL for default
                                   PA_STREAM_NOFLAGS,             //flags	Additional flags, or 0 for default
                                   NULL,                              //volume Initial volume, or NULL for default
                                   NULL);                             //sync_stream Synchronize this stream with the specified one, or NULL for a standalone stream
    }
    else {
        say("connecting record");
        //on definis le callback de lecture
        pa_stream_set_read_callback(stream,
                                    PulseDeviceASync::stream_read_callback,
                                    this);

        //on connecte le flux au contexte
        error = pa_stream_connect_record(stream,
                                         NULL, //source name, NULL for default
                                         NULL,
                                         PA_STREAM_NOFLAGS);

    }
    say("stream name: " + this->objectName());
    say("stream device: " + QString(pa_stream_get_device_name(stream)));
    say("stream device index: " + QString::number(pa_stream_get_device_index(stream)));

    if (error < 0) {
        say("return state: " + QString(pa_strerror(error)) + " / error num: " + QString::number(error));
        say("context state: " + stateToString(pa_context_get_state(context)));
        return false;
    }
    return true;
}

bool PulseDeviceASync::open(OpenMode mode) {
    say("opening context");

    if (!this->isValidSampleSepcs()) {
        say("invalid sample format: aborting");
        return false;
    }

    if (!context) {
        say("making context");
        if (!makeContext()) {
            say("failed to create context");
            return false;
        }
        say("context created.");
    }
    else say("using existing context");

    if (mode == QIODevice::ReadWrite) {
        say("readWrite mode is unsupported.");
        return false;
    }
    else if (mode == QIODevice::ReadOnly) {
        if (!readBuffer) {
            //Création du buffer de lecture (2Mb)
            readBuffer = new CircularBuffer(2097152,this->objectName(),this);

            //je définis la polituqe de buffer overflow sur expand: si la taille maxi du buffer est atteinte celui s'agrandira de lui même
            readBuffer->setOverflowPolicy(CircularBuffer::Expand);
            connect(readBuffer,SIGNAL(readyRead(int)),this,SIGNAL(readyRead()));
            connect(readBuffer,SIGNAL(debug(QString)),this,SLOT(say(QString)));
        }
    }

    /*ici on ment litéralement à QIODevice:
     * l'api de pulseaudio étant asynchrone et ne voulant pas faire de fonction blocante qui stoperais le open:
     * on lance la procédure de connection du contexte plus haut (via makeContext)
     * une fois créé et connecté, il crééra un stream via makeStream
     * alors la classe sera utilisable, il est aussi possible de vérifier si la classe est prete en attendant le signal readyWrite();
    */
    QIODevice::open(mode);
    return true;
}
bool PulseDeviceASync::isValidSampleSepcs() {
    if (ss.format == PA_SAMPLE_INVALID) return false;
    if (ss.channels == 0) return false;
    if (ss.rate == 0) return false;
    return true;
}

qint64 PulseDeviceASync::readData(char *data, qint64 maxlen) {
    if (!maxlen) return 0;
    qint64 available = bytesAvailable();

    //si on demande à lire plus que ce qui est dispo: on remet la valeur demandé à ce qui est lisible
    if (maxlen > available) maxlen = available;

    QByteArray sound = readBuffer->getCurrentPosData(maxlen);
    memcpy(data,sound.data(),maxlen);

    return maxlen;
}
qint64 PulseDeviceASync::writeData(const char *data, qint64 len) {
    if (!len) return 0;
    /* ici dans le cas ou le pointeur de stream n'est pas définis, on renvoi zero car
     * il est fort possible que le parent de la classe soit en train d'écrire avant que le contexte
     * ai eut le temps de se connecter au serveur pulseaudio (local ou distand)
     * pour ne pas empecher les écritures ulterieures il vaut mieux retourner 0 et non -1 (qui empecherais les lectures suivantes)
     * Todo: faire un buffer d'écriture qui garderais les infos manquées et les enveraient plus tard (au prix d'un lag, il faudrais rendre possible un choix)
     */
    if (!stream) return 0;
    const int writable = pa_stream_writable_size(stream);
    if (writable < len) len = writable;
    if (!len) return 0;


    int result = pa_stream_write(stream,
                    data,
                    len,
                    NULL,
                    0,
                    PA_SEEK_RELATIVE);

    if (!result) return len;
    return result;
}
void PulseDeviceASync::close() {
    QIODevice::close();
    say("closed");
}
void PulseDeviceASync::say(const QString message) {
    emit(debug("PulseAsync: " + message));
}
bool PulseDeviceASync::makeChannelMap(pa_channel_map* map) {
    if ((uint) format->getChannelsCount() > PA_CHANNELS_MAX) {
        say("error: max channels is: " + QString::number(PA_CHANNELS_MAX) + " current is: " + QString::number(format->getChannelsCount()));
        return false;
    }
    map->channels = format->getChannelsCount();
    pa_channel_map_init_auto(map,format->getChannelsCount(),PA_CHANNEL_MAP_ALSA);
    return true;
}
pa_sample_format PulseDeviceASync::getSampleFormat() {
    say("attemping to determinate format");
    if (!format) return PA_SAMPLE_INVALID;
    say("given sample size: " + QString::number(format->getSampleSize()));
    switch (format->getSampleSize()) {
        case 8:
            return PA_SAMPLE_U8;
        case 16:
            return PA_SAMPLE_S16NE;
        case 24:
            return PA_SAMPLE_S24NE;
        case 32:
            return PA_SAMPLE_S32NE;
        default:
            return PA_SAMPLE_INVALID;
    }
}
QList<int> PulseDeviceASync::getValidSamplesSize() {
    QList<int> rates;
    rates << 8 << 16 << 24 << 32;
    return rates;
}
QList<int> PulseDeviceASync::getDefaultSamplesRates() {
    QList<int> rates;
    rates << 5512  << 8000 << 11025 << 16000 << 22050 << 32000 << 44100
          << 48000 << 64000 << 88200 << 96000 << 176400 << 192000 << 384000;
    return rates;
}
void PulseDeviceASync::context_state_callback(pa_context *c, void *userdata) {
    PulseDeviceASync* p = (PulseDeviceASync*) userdata;
    //p->say("/!\\ callback event /!\\");
    if (!c) {
        p->say("invalid context");
        return;
    }

    switch (pa_context_get_state(c)) {
        case PA_CONTEXT_CONNECTING:
            p->say("connecting to pulseaudio");
            return;

        case PA_CONTEXT_AUTHORIZING:
            p->say("authorising");
            return;
        case PA_CONTEXT_SETTING_NAME:
            p->say("setting name");
            return;

        case PA_CONTEXT_READY:
            p->say("connection established");
            if (!p->makeStream()) {
                p->say("unable to create stream.");
                p->close();
            }
            else if (p->openMode() == QIODevice::WriteOnly) p->readyWrite();
            break;
        case PA_CONTEXT_TERMINATED:
            p->say("terminated");
            break;

        case PA_CONTEXT_FAILED:
            p->say("failed");
            p->close();
            break;
        default:
            p->say("Context error: " + QString::number(pa_context_errno(c)));
            break;
    }
}
void PulseDeviceASync::stream_state_callback(pa_stream* stream,void* userdata) {
    PulseDeviceASync* p = (PulseDeviceASync*) userdata;
    p->say("stream state callback called: " + QString::number((long) stream));
    switch (pa_stream_get_state(stream)) {
        case PA_STREAM_UNCONNECTED:
            p->say("stream not connected");
            break;
        case PA_STREAM_CREATING:
            p->say("stream creating state");
            break;
        case PA_STREAM_READY:
            p->say("stream is ready");
            break;
        case PA_STREAM_FAILED:
            p->say("stream failed state");
            p->close();
            break;
        case PA_STREAM_TERMINATED:
            p->say("stream terminated");
            p->close();
            break;
    }
}

QString PulseDeviceASync::stateToString(pa_context_state_t state) {
    QMap<pa_context_state_t,QString> states;
    states[PA_CONTEXT_CONNECTING] = "connecting";
    states[PA_CONTEXT_AUTHORIZING] = "authorising";
    states[PA_CONTEXT_SETTING_NAME] = "setting name";
    states[PA_CONTEXT_READY] = "ready";
    states[PA_CONTEXT_TERMINATED] = "terminated";
    states[PA_CONTEXT_FAILED] = "failed";

    QMap<pa_context_state_t,QString>::iterator i;
    for (i = states.begin() ; i != states.end() ; i++) {
        if (i.key() == state) return i.value();
    }
    return QString();
}
qint64 PulseDeviceASync::bytesAvailable() {
    //si on est en mode lecture: renvoi de 0 pour éviter un incident facheux (le pointeur readBuffer serait = à NULL)
    switch (this->openMode()) {
        case QIODevice::ReadWrite:
            return -1;
        case QIODevice::WriteOnly:
            return 0;
        case QIODevice::ReadOnly:
            return QIODevice::bytesAvailable() + readBuffer->getAvailableBytesCount();
    }
    return -1;
}
void PulseDeviceASync::setObjectName(const QString &name) {
    /* je surclasse cette fonction pour renomer le flux en meme temps que l'objet
     * ainsi on s'y retrouve plus facilement dans les flux sur pulseaudio dans le controleur de volume
     */
    QIODevice::setObjectName(name);
    if (stream) {
        pa_stream_set_name(stream,name.toLocal8Bit().data(),NULL,NULL);
        say("renaming stream to: " + name);
    }
}

void PulseDeviceASync::stream_read_callback(pa_stream *stream, size_t len, void *userdata) {
    /* Ceci est la fonction qui lis les données depuis le flux
     * elle remplis le readBuffer et c'est ce dernier qui envoi les readyRead()
     */
    if (!len) return;
    PulseDeviceASync* p = (PulseDeviceASync*) userdata;
    char* data;

    size_t availableBytes = pa_stream_readable_size(stream);

    //Contrairement à ce que dis la documentation de pulseaudio: la fonction retourne 0 si les données on été bien lues, sinon -1
    //mais en aucun cas la quantitée de données lues
    int result = pa_stream_peek(stream,
                                (const void**) &data,
                                &availableBytes);

    if (result < 0) p->say("error while reading audio from record stream");
    else if (!p->readBuffer->append(data,availableBytes)) p->say("error: unable to add audio to read buffer");
    pa_stream_drop(stream);
}

#endif
