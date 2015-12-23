#include "renderthread.h"

#include <QMutex>
#include <QThread>

#include <QOpenGLContext>
#include <QOpenGLFramebufferObject>
#include <QGuiApplication>
#include <QOffscreenSurface>

#include <QQuickWindow>
#include <QSGSimpleTextureNode>

#include "mapsrenderer.h"
#include "mapsrenderviewport.h"

RenderThread::RenderThread(const QSize &size)
    : surface(0)
    , context(0)
    , m_renderFbo(0)
    , m_displayFbo(0)
    , m_mapsRenderer(0)
    , m_size(size)
    , m_mapManager(0)
    , m_mapId("")
{
    MapsRenderViewport::threads << this;
    m_mapsRenderer = new MapsRenderer();
}

RenderThread::~RenderThread()
{
    delete m_mapsRenderer;
}

upns::MapManager *RenderThread::mapManager() const
{
    return m_mapManager;
}

QString RenderThread::mapId() const
{
    return m_mapId;
}

qreal RenderThread::width() const
{
    return m_size.width();
}

qreal RenderThread::height() const
{
    return m_size.height();
}

QMatrix4x4 RenderThread::matrix() const
{
    return m_matrix;
}

QString RenderThread::layerId() const
{
    return m_layerId;
}

void RenderThread::reloadMap()
{
    m_mapsRenderer->reloadMap();
}

void RenderThread::renderNext()
{
    context->makeCurrent(surface);

    if(m_renderFbo && m_renderFbo->size() != m_size)
    {
        delete m_renderFbo;
        m_renderFbo = NULL;
        delete m_displayFbo;
        m_displayFbo = NULL;
    }
    if (!m_renderFbo)
    {
        // Initialize the buffers and renderer
        QOpenGLFramebufferObjectFormat format;
        format.setAttachment(QOpenGLFramebufferObject::CombinedDepthStencil);
        m_renderFbo = new QOpenGLFramebufferObject(m_size, format);
        m_displayFbo = new QOpenGLFramebufferObject(m_size, format);
    }
    if( !m_mapsRenderer->isInitialized() )
    {
        m_mapsRenderer->setMapId( mapId().toULongLong() );
        m_mapsRenderer->setMapmanager( mapManager() );
        m_mapsRenderer->initialize();
    }

    m_renderFbo->bind();
    context->functions()->glViewport(0, 0, m_size.width(), m_size.height());

    m_mapsRenderer->render();

    // We need to flush the contents to the FBO before posting
    // the texture to the other thread, otherwise, we might
    // get unexpected results.
    context->functions()->glFlush();

    m_renderFbo->bindDefault();
    qSwap(m_renderFbo, m_displayFbo);

    Q_EMIT textureReady(m_displayFbo->texture(), m_size);
}

void RenderThread::shutDown()
{
    context->makeCurrent(surface);
    delete m_renderFbo;
    delete m_displayFbo;
    context->doneCurrent();
    delete context;

    // schedule this to be deleted only after we're done cleaning up
    surface->deleteLater();

    // Stop event processing, move the thread to GUI and make sure it is deleted.
    QCoreApplication* app = QGuiApplication::instance();
    QThread *guiThread = app->thread();
    moveToThread(guiThread);
    exit();
}

void RenderThread::setMapManager(upns::MapManager *mapManager)
{
    if (m_mapManager == mapManager)
        return;

    m_mapManager = mapManager;
    if(m_mapsRenderer)
        m_mapsRenderer->setMapmanager( mapManager );
    Q_EMIT mapManagerChanged(mapManager);
}

void RenderThread::setMapId(QString mapId)
{
    if (m_mapId == mapId)
        return;

    m_mapId = mapId;
    if(m_mapsRenderer)
        m_mapsRenderer->setMapId( mapId.toULongLong() );
    Q_EMIT mapIdChanged(mapId);
}

void RenderThread::setLayerId(QString layerId)
{
    if (m_layerId == layerId)
        return;

    m_layerId = layerId;
    if(m_mapsRenderer)
        m_mapsRenderer->setLayerId( layerId.toULongLong() );
    Q_EMIT layerIdChanged(layerId);
}

void RenderThread::setWidth(qreal width)
{
    if (m_size.width() == width)
        return;

    m_size.setWidth(width);
    Q_EMIT widthChanged(width);
}

void RenderThread::setHeight(qreal height)
{
    if (m_size.height() == height)
        return;

    m_size.setHeight(height);
    Q_EMIT heightChanged(height);
}

void RenderThread::setMatrix(QMatrix4x4 matrix)
{
    if (m_matrix == matrix)
        return;

    m_matrix = matrix;
    if(m_mapsRenderer)
        m_mapsRenderer->setMatrix( matrix );
    Q_EMIT matrixChanged(matrix);
}