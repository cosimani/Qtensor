#include "camerarunnable.h"

#include <QDebug>
#include <QResource>
#include <QFile>
#include <QDir>
#include <QMessageBox>
#include <QOpenGLExtraFunctions>
#include <QMatrix4x4>
#include <QPainter>

#include "texturebuffer.h"

#include <vector>

#include "scene.h"

CameraRunnable::CameraRunnable( CameraFilter * filter ) : m_filter(filter),
                                                          markerDetector( new aruco::MarkerDetector ),
                                                          cameraParameters( new aruco::CameraParameters )
{
    texture = new QOpenGLTexture( QOpenGLTexture::Target2D );
    texture->setMinificationFilter( QOpenGLTexture::Nearest );
    texture->setMagnificationFilter( QOpenGLTexture::Linear );
    texture->setFormat( QOpenGLTexture::RGBA8_UNorm );

    // Esto para leer :/CameraParameters.yml de los recursos y crear un archivo nuevo en el celular
    #define CAMERA_PARAMETERS_FILE_LOCAL "./CameraParameters.yml"

    QFile ymlFileResource( QResource( ":/CameraParameters.yml" ).absoluteFilePath() );
    ymlFileResource.open( QIODevice::ReadOnly | QIODevice::Text );
    QString content = QTextStream( &ymlFileResource ).readAll();

    // Creo un archivo nuevo para almacenarlo
    QFile ymlFileLocal( CAMERA_PARAMETERS_FILE_LOCAL );
    ymlFileLocal.open( QIODevice::WriteOnly | QIODevice::Text );

    QTextStream out( &ymlFileLocal );
    out << content;
    ymlFileLocal.close();
    // Aca ya esta creado el archivo local ./CameraParameters.yml

    cameraParameters->readFromXMLFile( CAMERA_PARAMETERS_FILE_LOCAL );

    if ( ! cameraParameters->isValid() )  {
        qDebug() << "Error con YML / No es valido. La App se cerrara";
    }


    // Esto para leer :/face.xml de los recursos y crear un archivo nuevo en el celular
    #define FACE_CLASSIFIER_FILE_LOCAL "./face.xml"

    QFile faceFileResource( QResource( ":/face.xml" ).absoluteFilePath() );
    faceFileResource.open( QIODevice::ReadOnly | QIODevice::Text );
    QString faceFileContent = QTextStream( &faceFileResource ).readAll();

    // Creo un archivo nuevo para almacenarlo
    QFile faceFileLocal( FACE_CLASSIFIER_FILE_LOCAL );
    faceFileLocal.open( QIODevice::WriteOnly | QIODevice::Text );

    QTextStream faceFileOut( &faceFileLocal );
    faceFileOut << faceFileContent;
    faceFileLocal.close();
    // Aca ya esta creado el archivo local ./face.xml

    frontalFaceClassifier = new CascadeClassifier( FACE_CLASSIFIER_FILE_LOCAL );




//    #define FACE_CLASSIFIER_FILE ":/face.xml"
//    #define FACE_CLASSIFIER_FILE_LOCAL "./face.xml"
//    #define SMILE_CLASSIFIER_FILE ":/smile.xml"


//    QResource face_xml( FACE_CLASSIFIER_FILE );
//    QFile face_xml_fileResource(face_xml.absoluteFilePath());

//    if ( ! face_xml_fileResource.open( QIODevice::ReadOnly | QIODevice::Text ) )  {
//        qDebug() << "No se pudo iniciar camara 2 / Problema con parametros de la camara";
//    }

//    QTextStream face_xml_in(&ymlFileResource);
//    QString face_xml_content = face_xml_in.readAll();


//    // Creo un archivo nuevo para almacenarlo
//    QFile face_xml_fileLocal( FACE_CLASSIFIER_FILE_LOCAL );
//    if (!face_xml_fileLocal.open(QIODevice::WriteOnly | QIODevice::Text))  {
//        qDebug() << "No se pudo iniciar camara / Problema con parametros de la camara";
//    }


//    QTextStream face_xml_out(&face_xml_fileLocal);
//    face_xml_out << face_xml_content;

//    face_xml_fileLocal.close();







//    QString faceClassifierFile( FACE_CLASSIFIER_FILE_LOCAL );
////    QString smileClassifierFile( SMILE_CLASSIFIER_FILE );

//    frontalFaceClassifier = new CascadeClassifier( faceClassifierFile.toStdString() );
////    smileClassifier = new CascadeClassifier( smileClassifierFile.toStdString() );



}

CameraRunnable::~CameraRunnable()
{
    qDebug()<<"{destructor FilterRunnable}";
}


/**
 * @brief FilterRunnable::run
 * @param input Para Desktop el pixelFormat = Format_YUV420P
 *              Para Samsung S8 pixelFormat = Format_BGR32
 *
 *        input Para Desktop el input->handleType() = NoHandle
 *              Para Samsung S8 input->handleType() = GLTextureHandle
 *
 *        input Para Desktop el input->width() height() bytesPerLine() = 640 480 0
 *              Para Samsung S8 input->width() height() bytesPerLine() = 1440 1080 5760 -> FrontalFace
 *              Para Samsung S8 input->width() height() bytesPerLine() = 1440 1080 5760 -> BackFace
 *              ( Raro que la camara frontal y la trasera sean con la misma resolucion, bueno, eso dio )
 *              Para Samsung S8 si dividimos 5760 / 1440 = 4 . Entonces, 4 canales, por eso es Format_BGR32
 *                  Este formato QVideoFrame::Format_BGR32 es using a 32-bit BGR format (0xBBGGRRff).
 *
 * @param surfaceFormat Para Desktop los valores de QVideoSurfaceFormat son:
 *              pixelFormat = Format_YUV420P  //  pixelAspectRatio = QSize(1, 1)
 *        surfaceFormat Para Android Samsung S8 los valores de QVideoSurfaceFormat son:
 *              pixelFormat = Format_BGR32  //  pixelAspectRatio = QSize(1, 1)
 *
 * @param flags Para Desktop: flags = QFlags(0x1)
 * @return
 */
QVideoFrame CameraRunnable::run( QVideoFrame *input,
                                 const QVideoSurfaceFormat &surfaceFormat,
                                 QVideoFilterRunnable::RunFlags flags )
{
    Q_UNUSED(surfaceFormat);
    Q_UNUSED(flags);

    if ( ! input->isValid() )
        return *input;

    if ( input->handleType() == QAbstractVideoBuffer::GLTextureHandle )  {

        using namespace cv;
        input->map(QAbstractVideoBuffer::ReadOnly);

        QImage image = this->wrapper( *input );

//        image = image.scaled( 640, 480 );

        image = image.convertToFormat( QImage::Format_RGB888 );


//        QPainter painter( &image );
//        painter.drawRect( 10, 10, 50, 50 );


        Mat mat( image.height(), image.width(), CV_8UC3, image.bits(), image.bytesPerLine() );

////        flip( mat, mat, 0 );  // eje x
//        flip( mat, mat, 1 );  // eje y
//        flip( mat, mat, -1 );  // ambos

//        transpose( mat, mat );
//        flip( mat, mat, 0 );

        Mat frame_gray;

        cv::cvtColor( mat, frame_gray, CV_BGR2GRAY );
        cv::equalizeHist( frame_gray, frame_gray );



        cv::rectangle( mat, cv::Rect( 10, 10, 50, 50 ), cv::Scalar( 0, 255, 0 ) );


        std::vector< cv::Rect > detectedFaces;

        frontalFaceClassifier->detectMultiScale( frame_gray, detectedFaces,
                                                 1.1, 2, 0 | CASCADE_SCALE_IMAGE, cv::Size( 150, 150 ) );

        for( unsigned int i = 0; i < detectedFaces.size(); i++ )
        {
            cv::Point center( detectedFaces[i].x + detectedFaces[i].width*0.5,
                              detectedFaces[i].y + detectedFaces[i].height*0.5 );

            cv::ellipse( mat, center, cv::Size( detectedFaces[i].width*0.5, detectedFaces[i].height*0.5),
                         0, 0, 360, cv::Scalar( 255, 0, 255 ), 4, 8, 0 );

//            QPainter painter( &image );
//            painter.drawRect( detectedFaces[i].x, detectedFaces[i].y, detectedFaces[i].width, detectedFaces[i].height );



        }

//        flip( mat, mat, 0 );

//        transpose(mat, mat);
//        flip(mat, mat, 1);



        input->unmap();

        texture->setData( image );
        texture->bind();

        return QVideoFrame(new TextureBuffer( texture->textureId() ), input->size(), input->pixelFormat() );
    }
    else  {
        return *input;
    }
}

QImage CameraRunnable::wrapper( const QVideoFrame & input )
{
    // Slow and inefficient path.
    // Ideally what's on the GPU should remain on the GPU, instead of readbacks like this.
    // Ver alguna opcion en: https://github.com/alpqr/qt-opencv-demo/blob/master/filter.cpp
    QImage image( input.width(), input.height(), QImage::Format_RGBA8888 );


    GLuint textureId = input.handle().toUInt();

    QOpenGLContext *ctx = QOpenGLContext::currentContext();
    QOpenGLFunctions *f = ctx->functions();

    f->glClearColor(0.0f, 0.0f, 1.0f, 1.0f);
    f->glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    f->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
    f->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );

    GLuint fbo;
    f->glGenFramebuffers( 1, &fbo );

    GLuint prevFbo;
    f->glGetIntegerv( GL_FRAMEBUFFER_BINDING, (GLint *) &prevFbo );

    f->glBindFramebuffer( GL_FRAMEBUFFER, fbo );

    f->glFramebufferTexture2D( GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, textureId, 0 );
    f->glReadPixels( 0, 0, input.width(), input.height(), GL_RGBA, GL_UNSIGNED_BYTE, image.bits() );


    f->glBindFramebuffer( GL_FRAMEBUFFER, prevFbo );

    return image;
}
