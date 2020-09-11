#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQuickImageProvider>
#include <QObject>
#include <QRecursiveMutex>
#include <QMutexLocker>
#include <QOpenGLTexture>

class SimpleImageProvider : public QQuickImageProvider
{
public:
    SimpleImageProvider() : QQuickImageProvider(Image) {}

    QImage requestImage(const QString &id, QSize *size, const QSize& /*requestedSize*/)
    {
        QMutexLocker locker(&m_mutex);
        QImage res;
        if (m_data.contains(id))
            res = m_data.value(id);
        if (size)
            *size = QSize(res.width(), res.height());
        return res;
    }

    void addImage(const QImage &image, const QString &name) {
        QMutexLocker locker(&m_mutex);
        m_data.insert(name, image);
    }

    QRecursiveMutex m_mutex;
    QMap<QString, QImage> m_data;
};

class ImageDiffer : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QVariant imageModel MEMBER m_imageModel NOTIFY imageModelChanged)
    Q_PROPERTY(float gain READ gain WRITE setGain)
public:
    ImageDiffer(QObject *parent = nullptr) : QObject(parent)
    {

    }
    ~ImageDiffer() {}

    float gain() const { return m_gain; }
    void setGain(float gain) {
        m_gain = gain;
        // rediff
        diff(m_pathA, m_pathB);
    }

    float channel(const QRgb &input, QOpenGLTexture::SwizzleValue ch) {
        switch (ch) {
            case QOpenGLTexture::RedValue: return qRed(input);
            case QOpenGLTexture::GreenValue: return qGreen(input);
            case QOpenGLTexture::BlueValue: return qBlue(input);
            default: return qRed(input);
        }
    }

    QImage diffChannel(const QImage &a, const QImage &b, QOpenGLTexture::SwizzleValue ch)
    {
        if (a.size() != b.size())
            return QImage();
        QImage res(a.size(), QImage::Format_RGB32);
        for (int y = 0; y < res.height(); ++y)
           for (int x = 0; x < res.width(); ++x) {
               float value = qBound<float>(0, 255.0f - (qAbs(channel(a.pixel(x,y), ch) -  channel(b.pixel(x,y), ch)) * m_gain) , 255.0f);
               res.setPixel(x,y, qRgb(value,value,value));
           }
        return res;
    }

    Q_INVOKABLE void diff(const QString pathA, const QString pathB)
    {
        // Load A
        m_pathA = pathA;
        // Load B
        m_pathB = pathB;
        if (m_pathA.isEmpty() || m_pathB.isEmpty()) return;
        QImage A(pathA);
        QImage B(pathB);
        SimpleImageProvider *p = provider();
        QString aName = QUrl(pathA).fileName();
        QString bName = QUrl(pathB).fileName();
        // diff A and B
        QString redName = aName+bName+"_red";
        QString greenName = aName+bName+"_green";
        QString blueName = aName+bName+"_blue";
        // store all to imageProvider
        QImage redDiff = diffChannel(A, B, QOpenGLTexture::RedValue);
        QImage greenDiff = diffChannel(A, B, QOpenGLTexture::GreenValue);
        QImage blueDiff = diffChannel(A, B, QOpenGLTexture::BlueValue);
        // set names to model
        p->addImage( A, aName);
        p->addImage( B, bName);
        p->addImage( redDiff, redName);
        p->addImage( greenDiff, greenName);
        p->addImage( blueDiff, blueName);
        QVariantList model;
        model.append("image://simpleImageProvider/"+aName);
        model.append("image://simpleImageProvider/"+bName);
        model.append("image://simpleImageProvider/"+redName);
        model.append("image://simpleImageProvider/"+greenName);
        model.append("image://simpleImageProvider/"+blueName);
        m_imageModel = QVariant();
        emit imageModelChanged();
        m_imageModel = model;
        emit imageModelChanged();

    }

signals:
    void imageModelChanged();

public:
    SimpleImageProvider *provider() {
        return static_cast<SimpleImageProvider *>(qmlEngine(this)->imageProvider("simpleImageProvider"));
    }

    QQmlEngine *engine;
    QVariant m_imageModel;
    float m_gain = 1.0;
    QString m_pathA;
    QString m_pathB;
};

#include "main.moc"

int main(int argc, char *argv[])
{
    QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);

    QGuiApplication app(argc, argv);

    QQmlApplicationEngine engine;

    engine.addImageProvider(QLatin1String("simpleImageProvider"), new SimpleImageProvider());
    const QUrl url(QStringLiteral("qrc:/main.qml"));
    QObject::connect(&engine, &QQmlApplicationEngine::objectCreated,
                     &app, [url](QObject *obj, const QUrl &objUrl) {
        if (!obj && url == objUrl)
            QCoreApplication::exit(-1);
    }, Qt::QueuedConnection);
    qmlRegisterType<ImageDiffer>("ImageDiffer", 1, 0, "ImageDiffer");
    engine.load(url);

    return app.exec();
}
