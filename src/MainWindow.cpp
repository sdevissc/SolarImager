#include "MainWindow.h"
#include "Theme.h"
#include "CameraInterface.h"
#include "BaslerCamera.h"
#include "ZwoCameraInterface.h"
#include "PlayerOneCameraInterface.h"
#include "PreviewWidget.h"
#include "SerPlayerDialog.h"
#include "FrameGrabber.h"
#include "HistogramWidget.h"
#include "SSMReader.h"
#include "SeePlot.h"

#include <QApplication>
#include <QCloseEvent>
#include <QCheckBox>
#include <QDebug>
#include <QComboBox>
#include <QDateTime>
#include <QDir>
#include <QDoubleSpinBox>
#include <QFileDialog>
#include <QFrame>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QProgressBar>
#include <QPushButton>
#include <QScrollArea>
#include <QSerialPortInfo>
#include <QSlider>
#include <QSpinBox>
#include <QStatusBar>
#include <QTimer>
#include <fitsio.h>
#include <QVBoxLayout>
#include <QGridLayout>
#include <algorithm>
#include <cmath>

// ── helpers ───────────────────────────────────────────────────────────────────

static QPushButton *makeToggleBtn(const QString &text, int w = 0)
{
    auto *btn = new QPushButton(text);
    btn->setCheckable(true);
    btn->setFixedHeight(28);
    if (w > 0) btn->setFixedWidth(w);
    btn->setStyleSheet(Theme::toggleButtonStyle());
    return btn;
}

// ── constructor / destructor ──────────────────────────────────────────────────

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    setWindowTitle("Solar Imaging Console – Basler 1920-155 + Airylab SSM");
    setMinimumSize(1600, 900);

    // Camera brand selector — determines which SDK is used on connect
    // Populated in buildUi(); actual object created in onCameraConnectToggled()
    m_camera = nullptr;

    buildUi();

    // ── ROI drag-selection ────────────────────────────────────────────────────
    connect(m_previewLabel, &PreviewWidget::roiSelected,
            this, [this](int x, int y, int w, int h) {
        if (!m_camera || !m_camera->isOpen()) return;
        m_spinOffsetX->blockSignals(true); m_spinOffsetX->setValue(x); m_spinOffsetX->blockSignals(false);
        m_spinOffsetY->blockSignals(true); m_spinOffsetY->setValue(y); m_spinOffsetY->blockSignals(false);
        m_spinWidth->blockSignals(true);   m_spinWidth->setValue(w);   m_spinWidth->blockSignals(false);
        m_spinHeight->blockSignals(true);  m_spinHeight->setValue(h);  m_spinHeight->blockSignals(false);
        m_camera->setRoi(x, y, w, h);
        m_btnClearRoi->setEnabled(true);
    });

    // ── Camera button wiring ──────────────────────────────────────────────────
    connect(m_btnConnect, &QPushButton::toggled,
            this, &MainWindow::onCameraConnectToggled);
    connect(m_btnLive,   &QPushButton::toggled,
            this, &MainWindow::onLiveToggled);
    connect(m_btnRecord, &QPushButton::toggled,
            this, &MainWindow::onRecordToggled);

    // ── Exposure ──────────────────────────────────────────────────────────────
    connect(m_spinExposure, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, [this](double v) { if (m_camera) m_camera->setExposure(v); });
    // Slider ↔ spinbox (mapped: slider 1–1000 → 100–1e6 µs logarithmically)
    connect(m_sliderExposure, &QSlider::valueChanged, this, [this](int v) {
        double us = 100.0 * std::pow(10.0, (v - 1) / 999.0 * 4.0);
        m_spinExposure->blockSignals(true);
        m_spinExposure->setValue(us);
        m_spinExposure->blockSignals(false);
        if (m_camera) m_camera->setExposure(us);
    });
    // ── Gain ──────────────────────────────────────────────────────────────────
    connect(m_spinGain, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, [this](double v) {
        m_sliderGain->blockSignals(true);
        m_sliderGain->setValue(static_cast<int>(v * 10));
        m_sliderGain->blockSignals(false);
        if (m_camera) m_camera->setGain(v);
    });
    connect(m_sliderGain, &QSlider::valueChanged, this, [this](int v) {
        m_spinGain->blockSignals(true);
        m_spinGain->setValue(v / 10.0);
        m_spinGain->blockSignals(false);
        if (m_camera) m_camera->setGain(v / 10.0);
    });

    // ── ROI / Offset ──────────────────────────────────────────────────────────
    connect(m_spinOffsetX, QOverload<int>::of(&QSpinBox::valueChanged),
            this, [this](int) {
        if (m_camera) m_camera->setOffset(m_spinOffsetX->value(), m_spinOffsetY->value()); });
    connect(m_spinOffsetY, QOverload<int>::of(&QSpinBox::valueChanged),
            this, [this](int) {
        if (m_camera) m_camera->setOffset(m_spinOffsetX->value(), m_spinOffsetY->value()); });
    connect(m_spinWidth,  QOverload<int>::of(&QSpinBox::valueChanged),
            this, [this](int) {
        if (m_camera) m_camera->setRoi(m_spinOffsetX->value(), m_spinOffsetY->value(),
                          m_spinWidth->value(), m_spinHeight->value()); });
    connect(m_spinHeight, QOverload<int>::of(&QSpinBox::valueChanged),
            this, [this](int) {
        if (m_camera) m_camera->setRoi(m_spinOffsetX->value(), m_spinOffsetY->value(),
                          m_spinWidth->value(), m_spinHeight->value()); });
    connect(m_btnClearRoi, &QPushButton::clicked, this, [this] {
        m_camera->clearRoi();
        m_previewLabel->clearRoi();
        m_spinOffsetX->blockSignals(true); m_spinOffsetX->setValue(0); m_spinOffsetX->blockSignals(false);
        m_spinOffsetY->blockSignals(true); m_spinOffsetY->setValue(0); m_spinOffsetY->blockSignals(false);
        m_spinWidth->blockSignals(true);   m_spinWidth->setValue(m_camera->caps().widthMax);  m_spinWidth->blockSignals(false);
        m_spinHeight->blockSignals(true);  m_spinHeight->setValue(m_camera->caps().heightMax); m_spinHeight->blockSignals(false);
        m_btnClearRoi->setEnabled(false);
    });

    // ── Pixel format ──────────────────────────────────────────────────────────
    connect(m_comboFormat, &QComboBox::currentTextChanged,
            this, [this](const QString &fmt) {
                if (m_camera) m_camera->setPixelFormat(fmt); });

    // ── Display levels ────────────────────────────────────────────────────────
    connect(m_sliderBlack, &QSlider::valueChanged, this, [this](int v) {
        if (v >= m_sliderWhite->value()) {
            m_sliderBlack->blockSignals(true);
            m_sliderBlack->setValue(m_sliderWhite->value() - 1);
            v = m_sliderWhite->value() - 1;
            m_sliderBlack->blockSignals(false);
        }
        m_lblBlack->setText(QString::number(v));
    });
    connect(m_sliderWhite, &QSlider::valueChanged, this, [this](int v) {
        if (v <= m_sliderBlack->value()) {
            m_sliderWhite->blockSignals(true);
            m_sliderWhite->setValue(m_sliderBlack->value() + 1);
            v = m_sliderBlack->value() + 1;
            m_sliderWhite->blockSignals(false);
        }
        m_lblWhite->setText(QString::number(v));
    });

    // ── Histogram log scale ───────────────────────────────────────────────────
    connect(m_chkHistLog, &QCheckBox::stateChanged, this, [this](int s) {
        m_histogramWidget->setLogScale(s == Qt::Checked);
    });

    // ── Save directory ────────────────────────────────────────────────────────
    connect(m_btnSaveDir, &QPushButton::clicked, this, [this] {
        QString dir = QFileDialog::getExistingDirectory(
            this, "Select save directory", m_lblSaveDir->text());
        if (!dir.isEmpty()) m_lblSaveDir->setText(dir);
    });

    // ── SSM ───────────────────────────────────────────────────────────────────
    connect(m_btnSsm, &QPushButton::toggled,
            this, &MainWindow::onSsmConnectToggled);
    connect(m_spinSsmThreshold,
            QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, [this](double v) { m_ssmPlot->setThreshold(v); });

    connect(m_btnSsmLog, &QPushButton::toggled,
            this, [this](bool checked) {
        if (checked) {
            const QString path = m_lblSaveDir->text() + "/ssm_"
                + QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss") + ".csv";
            m_ssmLogFile.setFileName(path);
            if (!m_ssmLogFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
                m_btnSsmLog->setChecked(false);
                statusBar()->showMessage("SSM log: cannot open " + path);
                return;
            }
            m_ssmLogStream.setDevice(&m_ssmLogFile);
            m_ssmLogStream << "UTC,InputLevel,Seeing_arcsec\n";
            m_ssmLogStream.flush();
            m_lblSsmLogPath->setText(path);
            statusBar()->showMessage("SSM logging → " + path);
        } else {
            if (m_ssmLogFile.isOpen()) {
                m_ssmLogStream.flush();
                m_ssmLogFile.close();
            }
            statusBar()->showMessage("SSM logging stopped");
        }
    });

    // SSM port list refresh every 5 s
    m_ssmPortTimer = new QTimer(this);
    connect(m_ssmPortTimer, &QTimer::timeout,
            this, &MainWindow::refreshSsmPorts);
    m_ssmPortTimer->start(5000);
    refreshSsmPorts();

    statusBar()->showMessage("Ready – select a camera brand and click Connect");
}

MainWindow::~MainWindow() = default;

void MainWindow::closeEvent(QCloseEvent *event)
{
    if (m_grabber) {
        if (m_camera) m_camera->stopGrabbing();
        m_grabber->stop();
        m_grabber->wait(5000);
        delete m_grabber;
        m_grabber = nullptr;
    }
    if (m_camera) {
        m_camera->close();
        m_camera->deleteLater();
        m_camera = nullptr;
    }
    event->accept();
}

// ── Camera slots ──────────────────────────────────────────────────────────────

void MainWindow::onCameraConnectToggled(bool checked)
{
    if (checked) {
        // Create the correct camera implementation
        if (m_camera) {
            m_camera->deleteLater();
            m_camera = nullptr;
        }
        const QString brand = m_comboCameraBrand->currentText();
        if (brand == "Basler")
            m_camera = new BaslerCamera(this);
        else if (brand == "ZWO")
            m_camera = new ZwoCamera(this);
        else if (brand == "Player One")
            m_camera = new PlayerOneCamera(this);
        else {
            statusBar()->showMessage("Unknown camera brand: " + brand);
            m_btnConnect->setChecked(false);
            return;
        }
        connect(m_camera, &CameraInterface::errorOccurred,
                this, &MainWindow::onCameraError);

        bool ok = m_camera->open();
        if (m_camera->isSimulated()) {
            m_lblCameraInfo->setText("⚠ Simulation mode – no camera found");
            m_lblCameraInfo->setStyleSheet("color:#b35c00; font-weight:bold;");
        } else if (ok) {
            m_lblCameraInfo->setText(m_camera->modelName());
            m_lblCameraInfo->setStyleSheet("color:#1a7a1a; font-weight:bold;");
        } else {
            m_lblCameraInfo->setText("Connection failed");
            m_lblCameraInfo->setStyleSheet("color:#c0392b;");
            m_btnConnect->setChecked(false);
            return;
        }

        // Update format combo to match this camera's supported formats
        m_comboFormat->blockSignals(true);
        m_comboFormat->clear();
        m_comboFormat->addItems(m_camera->supportedFormats());
        m_comboFormat->setCurrentIndex(0);
        m_comboFormat->blockSignals(false);

        // Update UI ranges to match this camera's actual capabilities
        const auto caps = m_camera->caps();
        m_spinExposure->blockSignals(true);
        m_spinExposure->setRange(caps.exposureMin, caps.exposureMax);
        m_spinExposure->setValue(10000.0);  // default 10ms
        m_spinExposure->blockSignals(false);

        // Sync slider to match 10000µs on the logarithmic scale
        // Inverse of: us = 100.0 * pow(10, (v-1)/999.0 * 4.0)
        // => v = 1 + 999 * log10(us/100) / 4
        {
            const double us = 10000.0;
            const int sliderPos = static_cast<int>(
                1.0 + 999.0 * std::log10(us / 100.0) / 4.0);
            m_sliderExposure->blockSignals(true);
            m_sliderExposure->setValue(std::clamp(sliderPos, 1, 1000));
            m_sliderExposure->blockSignals(false);
        }

        m_spinGain->blockSignals(true);
        m_spinGain->setRange(caps.gainMin, caps.gainMax);
        m_spinGain->setValue(caps.gainMin);
        m_spinGain->blockSignals(false);

        m_sliderGain->blockSignals(true);
        m_sliderGain->setRange(static_cast<int>(caps.gainMin * 10),
                               static_cast<int>(caps.gainMax * 10));
        m_sliderGain->setValue(static_cast<int>(caps.gainMin * 10));
        m_sliderGain->blockSignals(false);

        m_spinOffsetX->setRange(0, caps.widthMax  - 64);
        m_spinOffsetY->setRange(0, caps.heightMax - 64);
        m_spinWidth->setRange(64,  caps.widthMax);
        m_spinWidth->setValue(caps.widthMax);
        m_spinHeight->setRange(64, caps.heightMax);
        m_spinHeight->setValue(caps.heightMax);

        statusBar()->showMessage("Camera connected: " + m_camera->modelName());
        // Auto-start live view — block signals to avoid calling onLiveToggled twice
        m_btnLive->blockSignals(true);
        m_btnLive->setChecked(true);
        m_btnLive->blockSignals(false);
        onLiveToggled(true);
    } else {
        if (m_grabber) {
            if (m_camera) m_camera->stopGrabbing();
            m_grabber->stop();
            m_grabber->wait(5000);
            delete m_grabber;
            m_grabber = nullptr;
        }
        if (m_camera) {
            m_camera->close();
            m_camera->deleteLater();
            m_camera = nullptr;
        }
        m_lblCameraInfo->setText("Not connected");
        m_lblCameraInfo->setStyleSheet("");
        m_btnLive->blockSignals(true);
        m_btnLive->setChecked(false);
        m_btnLive->blockSignals(false);
        m_btnRecord->setChecked(false);
        statusBar()->showMessage("Camera disconnected");
    }
}

void MainWindow::onLiveToggled(bool checked)
{
    if (!m_camera || !m_camera->isOpen()) {
        m_btnLive->setChecked(false);
        statusBar()->showMessage("Connect camera first");
        return;
    }
    if (checked) {
        // Clear the placeholder text
        m_previewLabel->setText("");
        m_previewLabel->setStyleSheet("");

        m_grabber = new FrameGrabber(m_camera, this);
        connect(m_grabber, &FrameGrabber::frameReady,
                this, &MainWindow::onFrameReady);
        connect(m_grabber, &FrameGrabber::recordingStats,
                this, &MainWindow::onRecordingStats);
        connect(m_grabber, &FrameGrabber::recordingFinished,
                this, &MainWindow::onRecordingFinished);
        m_grabber->start();
        statusBar()->showMessage("Live view started");
    } else {
        if (m_grabber) {
            if (m_camera) m_camera->stopGrabbing();
            m_grabber->stop();
            m_grabber->wait(5000);
            delete m_grabber;
            m_grabber = nullptr;
        }
        m_previewLabel->setText("Live view stopped");
        m_previewLabel->setStyleSheet("color:#aaa; font-size:18px;");
        m_previewLabel->setPixmap(QPixmap());
        m_btnRecord->setChecked(false);
        statusBar()->showMessage("Live view stopped");
    }
}

void MainWindow::onRecordToggled(bool checked)
{
    if (!m_grabber) {
        m_btnRecord->setChecked(false);
        return;
    }
    if (checked) {
        QString dir  = m_lblSaveDir->text();
        QString name = m_txtFilename->text().trimmed();
        if (name.isEmpty())
            name = "capture_" + QDateTime::currentDateTime()
                                    .toString("yyyyMMdd_HHmmss");
        if (!name.endsWith(".ser", Qt::CaseInsensitive))
            name += ".ser";
        QString path = dir + "/" + name;
        // Determine bit depth from current pixel format selection
        const QString fmt = m_comboFormat->currentText();
        const int bitDepth = (fmt == "RAW16" || fmt == "Mono12") ? 16 : 8;
        m_grabber->startRecording(path, m_spinFrames->value(), bitDepth);
        m_progressBar->setMaximum(m_spinFrames->value());
        m_progressBar->setValue(0);
        m_progressBar->setVisible(true);
        statusBar()->showMessage("Recording → " + path);
    } else {
        m_grabber->stopRecording();
        m_progressBar->setVisible(false);
    }
}

void MainWindow::onFrameReady(QImage img)
{
    // Feed histogram with raw image (before display levels)
    m_histogramWidget->updateFromImage(img);

    // Update PreviewWidget's image size for ROI coordinate mapping
    m_previewLabel->setImageSize(img.width(), img.height());    // ── Build display LUT (black/white point + palette) ───────────────────
    // This affects display only — recording uses the raw image above.
    int black = m_sliderBlack->value();
    int white = m_sliderWhite->value();
    const QString palette = m_comboPalette->currentText();

    // Step 1: black/white stretch → 0-255
    uchar stretch[256];
    {
        const float scale = 255.0f / std::max(1, white - black);
        for (int i = 0; i < 256; ++i) {
            int v = static_cast<int>((i - black) * scale);
            stretch[i] = static_cast<uchar>(std::clamp(v, 0, 255));
        }
    }

    // Step 2: palette mapping → QRgb LUT
    QRgb lut[256];
    if (palette == "Inverted") {
        for (int i = 0; i < 256; ++i) {
            uchar v = 255 - stretch[i];
            lut[i] = qRgb(v, v, v);
        }
    } else if (palette == "Rainbow") {
        // black→blue→cyan→green→yellow→red→white (5 segments)
        for (int i = 0; i < 256; ++i) {
            uchar s = stretch[i];
            int r, g, b;
            if      (s < 51)  { r=0;       g=0;       b=s*5; }
            else if (s < 102) { r=0;       g=(s-51)*5; b=255; }
            else if (s < 153) { r=0;       g=255;     b=255-(s-102)*5; }
            else if (s < 204) { r=(s-153)*5; g=255;   b=0; }
            else              { r=255;     g=255-(s-204)*5; b=0; }
            lut[i] = qRgb(std::clamp(r,0,255), std::clamp(g,0,255), std::clamp(b,0,255));
        }
    } else if (palette == "Red Hot") {
        // black→red→yellow→white (3 segments)
        for (int i = 0; i < 256; ++i) {
            uchar s = stretch[i];
            int r, g, b;
            if      (s < 85)  { r=s*3;  g=0;         b=0; }
            else if (s < 170) { r=255;  g=(s-85)*3;  b=0; }
            else              { r=255;  g=255;        b=(s-170)*3; }
            lut[i] = qRgb(std::clamp(r,0,255), std::clamp(g,0,255), std::clamp(b,0,255));
        }
    } else if (palette == "Cool") {
        // black→blue→cyan→white
        for (int i = 0; i < 256; ++i) {
            uchar s = stretch[i];
            int r, g, b;
            if      (s < 128) { r=0;      g=0;      b=s*2; }
            else              { r=0;      g=(s-128)*2; b=255; }
            lut[i] = qRgb(std::clamp(r,0,255), std::clamp(g,0,255), std::clamp(b,0,255));
        }
    } else {
        // Original — grayscale with stretch
        for (int i = 0; i < 256; ++i) {
            uchar v = stretch[i];
            lut[i] = qRgb(v, v, v);
        }
    }

    // Apply LUT to a display copy
    QImage display(img.width(), img.height(), QImage::Format_RGB32);
    if (img.format() == QImage::Format_Grayscale8) {
        for (int y = 0; y < img.height(); ++y) {
            const uchar *src = img.constScanLine(y);
            QRgb        *dst = reinterpret_cast<QRgb*>(display.scanLine(y));
            for (int x = 0; x < img.width(); ++x)
                dst[x] = lut[src[x]];
        }
    } else if (img.format() == QImage::Format_Grayscale16) {
        // Downscale 16→8 for display then apply LUT
        for (int y = 0; y < img.height(); ++y) {
            const uint16_t *src = reinterpret_cast<const uint16_t*>(img.constScanLine(y));
            QRgb           *dst = reinterpret_cast<QRgb*>(display.scanLine(y));
            for (int x = 0; x < img.width(); ++x)
                dst[x] = lut[src[x] >> 8];
        }
    } else {
        // RGB — apply stretch to green channel as luminance proxy, keep color
        display = img.convertToFormat(QImage::Format_RGB32);
        if (palette != "Original") {
            for (int y = 0; y < display.height(); ++y) {
                QRgb *row = reinterpret_cast<QRgb*>(display.scanLine(y));
                for (int x = 0; x < display.width(); ++x) {
                    uchar luma = static_cast<uchar>(qGreen(row[x]));
                    row[x] = lut[luma];
                }
            }
        }
    }

    // Parse zoom level from combo ("Fit" = fit to viewport, else percentage)
    const QString zoomText = m_comboZoom->currentText();
    const QSize   vpSize   = m_scrollArea->viewport()->size();
    QPixmap pix = QPixmap::fromImage(display);

    if (zoomText == "Fit" || zoomText.isEmpty()) {
        pix = pix.scaled(vpSize, Qt::KeepAspectRatio, Qt::FastTransformation);
        m_previewLabel->setAlignment(Qt::AlignCenter);
        m_previewLabel->resize(vpSize);
    } else {
        const double factor = zoomText.left(zoomText.indexOf('%')).toDouble() / 100.0;
        const int scaledW = static_cast<int>(img.width()  * factor);
        const int scaledH = static_cast<int>(img.height() * factor);
        pix = pix.scaled(scaledW, scaledH, Qt::KeepAspectRatio, Qt::FastTransformation);
        m_previewLabel->setAlignment(Qt::AlignCenter);
        m_previewLabel->resize(std::max(scaledW, vpSize.width()),
                               std::max(scaledH, vpSize.height()));
    }

    m_previewLabel->setPixmap(pix);
    m_previewLabel->update();
}

void MainWindow::onRecordingStats(double fps, double mbps, int framesDone)
{
    m_lblFps->setText(QString("fps: %1").arg(fps, 0, 'f', 1));
    m_lblMbps->setText(QString("MB/s: %1").arg(mbps, 0, 'f', 1));
    if (m_progressBar->isVisible())
        m_progressBar->setValue(framesDone);
}

void MainWindow::onRecordingFinished()
{
    m_btnRecord->setChecked(false);
    m_progressBar->setVisible(false);
    statusBar()->showMessage("Recording finished");
}

void MainWindow::onSnapFrame()
{
    if (!m_grabber || !m_camera || !m_camera->isOpen()) {
        statusBar()->showMessage("Connect camera first");
        return;
    }

    // Grab the current raw frame directly from the camera
    QImage frame = m_camera->grabFrame();
    if (frame.isNull()) {
        statusBar()->showMessage("Snap failed — no frame available");
        return;
    }

    // Build filename: same directory as SER recordings, timestamped
    const QString dir  = m_lblSaveDir->text();
    const QString name = "snap_" + QDateTime::currentDateTime()
                                       .toString("yyyyMMdd_HHmmss") + ".fits";
    const QString path = dir + "/" + name;

    // Write FITS file
    fitsfile *fptr = nullptr;
    int       status = 0;

    // cfitsio requires '!' prefix to overwrite existing files
    const QString fitsPath = "!" + path;
    fits_create_file(&fptr, fitsPath.toLocal8Bit().constData(), &status);
    if (status) {
        char errMsg[80];
        fits_get_errstatus(status, errMsg);
        statusBar()->showMessage(QString("FITS create failed: %1").arg(errMsg));
        return;
    }

    const int W = frame.width();
    const int H = frame.height();

    if (frame.format() == QImage::Format_Grayscale16) {
        // 16-bit mono
        long naxes[2] = { W, H };
        fits_create_img(fptr, SHORT_IMG, 2, naxes, &status);

        // FITS stores rows bottom-up — flip vertically
        QVector<uint16_t> rowBuf(W);
        for (int y = H - 1; y >= 0; --y) {
            const uint16_t *src = reinterpret_cast<const uint16_t*>(frame.constScanLine(y));
            std::copy(src, src + W, rowBuf.data());
            long fpixel[2] = { 1, H - y };
            fits_write_pix(fptr, TUSHORT, fpixel, W, rowBuf.data(), &status);
        }
    } else if (frame.format() == QImage::Format_Grayscale8) {
        // 8-bit mono
        long naxes[2] = { W, H };
        fits_create_img(fptr, BYTE_IMG, 2, naxes, &status);

        QVector<uint8_t> rowBuf(W);
        for (int y = H - 1; y >= 0; --y) {
            const uchar *src = frame.constScanLine(y);
            std::copy(src, src + W, rowBuf.data());
            long fpixel[2] = { 1, H - y };
            fits_write_pix(fptr, TBYTE, fpixel, W, rowBuf.data(), &status);
        }
    } else {
        // Convert to grayscale8 for other formats
        QImage gray = frame.convertToFormat(QImage::Format_Grayscale8);
        long naxes[2] = { W, H };
        fits_create_img(fptr, BYTE_IMG, 2, naxes, &status);

        QVector<uint8_t> rowBuf(W);
        for (int y = H - 1; y >= 0; --y) {
            const uchar *src = gray.constScanLine(y);
            std::copy(src, src + W, rowBuf.data());
            long fpixel[2] = { 1, H - y };
            fits_write_pix(fptr, TBYTE, fpixel, W, rowBuf.data(), &status);
        }
    }

    // Write useful keywords
    const QString dateObs = QDateTime::currentDateTimeUtc()
                                .toString("yyyy-MM-ddTHH:mm:ss.zzz");
    double expUs = m_spinExposure->value();
    fits_write_key(fptr, TSTRING, "INSTRUME",
                   const_cast<char*>(m_camera->modelName().toLocal8Bit().constData()),
                   "Camera model", &status);
    fits_write_key(fptr, TDOUBLE, "EXPTIME", &expUs,
                   "Exposure time (us)", &status);
    fits_write_key(fptr, TSTRING, "DATE-OBS",
                   const_cast<char*>(dateObs.toLocal8Bit().constData()),
                   "UTC observation date", &status);

    fits_close_file(fptr, &status);

    if (status) {
        char errMsg[80];
        fits_get_errstatus(status, errMsg);
        statusBar()->showMessage(QString("FITS write failed: %1").arg(errMsg));
    } else {
        statusBar()->showMessage("Snap saved → " + path);
        qInfo() << "[Snap] Saved" << path;
    }
}

// ── SSM slots ─────────────────────────────────────────────────────────────────

void MainWindow::refreshSsmPorts()
{
    QString current = m_comboSsmPort->currentText();
    m_comboSsmPort->blockSignals(true);
    m_comboSsmPort->clear();

    const auto ports = QSerialPortInfo::availablePorts();
    for (const auto &info : ports) {
        const QString name = info.portName();
        if (name.contains("USB", Qt::CaseInsensitive) ||
            name.contains("ACM", Qt::CaseInsensitive))
            m_comboSsmPort->addItem("/dev/" + name);
    }
    if (m_comboSsmPort->count() == 0)
        m_comboSsmPort->addItem("(none)");
    else if (m_comboSsmPort->findText(current) >= 0)
        m_comboSsmPort->setCurrentText(current);

    m_comboSsmPort->blockSignals(false);
}

void MainWindow::onCameraError(const QString &msg)
{
    statusBar()->showMessage("Camera error: " + msg);
}

void MainWindow::onSsmConnectToggled(bool checked)
{
    if (checked) {
        QString port = m_comboSsmPort->currentText();
        if (port == "(none)") {
            m_btnSsm->setChecked(false);
            statusBar()->showMessage("SSM: no serial port selected");
            return;
        }
        int baud = m_comboSsmBaud->currentText().toInt();
        m_ssmHistory.clear();
        m_ssmTriggerCount = 0;
        m_ssmConsecutive  = 0;

        m_ssmReader = new SSMReader(port, baud, this);
        connect(m_ssmReader, &SSMReader::newSample,
                this, &MainWindow::onSsmNewSample);
        connect(m_ssmReader, &SSMReader::rawLine,
                this, &MainWindow::onSsmRawLine);
        connect(m_ssmReader, &SSMReader::error,
                this, &MainWindow::onSsmError);
        connect(m_ssmReader, &SSMReader::connected,
                this, &MainWindow::onSsmConnected);
        connect(m_ssmReader, &SSMReader::disconnected,
                this, &MainWindow::onSsmDisconnected);
        m_ssmReader->start();
    } else {
        if (m_ssmReader) {
            m_ssmReader->stop();
            m_ssmReader->wait(3000);
            m_ssmReader->deleteLater();
            m_ssmReader = nullptr;
        }
        m_btnSsm->setChecked(false);
        statusBar()->showMessage("SSM disconnected");
    }
}

void MainWindow::onSsmConnected()
{
    m_btnSsm->setChecked(true);
    statusBar()->showMessage("SSM connected on " + m_comboSsmPort->currentText());
}

void MainWindow::onSsmDisconnected()
{
    m_btnSsm->setChecked(false);
    // Reset display to avoid stale values on next connect
    m_lblSsmCurrent->setText("—");
    m_lblSsmCurrent->setStyleSheet("font-weight:bold; font-size:15px; color:#555;");
    m_lblSsmInput->setText("—");
    m_lblSsmInput->setStyleSheet("font-weight:bold; font-size:13px;");
    m_lblSsmMean->setText("—");
    if (m_ssmLogFile.isOpen()) {
        m_ssmLogStream.flush();
        m_ssmLogFile.close();
        m_btnSsmLog->setChecked(false);
        m_lblSsmLogPath->setText("No file");
    }
}

void MainWindow::onSsmError(const QString &msg)
{
    if (m_ssmReader) {
        m_ssmReader->stop();
        m_ssmReader->wait(3000);
        m_ssmReader->deleteLater();
        m_ssmReader = nullptr;
    }
    m_btnSsm->setChecked(false);
    m_lblSsmTrigger->setText("Error: " + msg);
    m_lblSsmTrigger->setStyleSheet("color:#c0392b; font-size:10px;");
    statusBar()->showMessage("SSM error: " + msg);
}

void MainWindow::onSsmNewSample(double inputLevel, double seeing)
{
    m_ssmHistory.append(seeing);

    // CSV logging
    if (m_ssmLogFile.isOpen()) {
        const QString utc = QDateTime::currentDateTimeUtc()
                                .toString("yyyy-MM-ddTHH:mm:ss.zzz");
        m_ssmLogStream << utc << ","
                       << QString::number(inputLevel, 'f', 4) << ","
                       << QString::number(seeing,     'f', 3) << "\n";
        m_ssmLogStream.flush();
    }

    // Feed the plot
    double ts = QDateTime::currentDateTime().toSecsSinceEpoch();
    m_ssmPlot->addPoint(ts, seeing);

    // Update seeing display
    QString col = (seeing <= m_spinSsmThreshold->value())
                  ? "#1a7a1a" : "#c0392b";
    m_lblSsmCurrent->setText(QString("%1\"").arg(seeing, 0, 'f', 2));
    m_lblSsmCurrent->setStyleSheet(
        QString("font-weight:bold; font-size:15px; color:%1;").arg(col));

    // Input level
    if (inputLevel < 0.5) {
        m_lblSsmInput->setText(QString("%1  ⚠ Low").arg(inputLevel, 0, 'f', 3));
        m_lblSsmInput->setStyleSheet("font-weight:bold; font-size:13px; color:#c0392b;");
    } else {
        m_lblSsmInput->setText(QString::number(inputLevel, 'f', 3));
        m_lblSsmInput->setStyleSheet("font-weight:bold; font-size:13px; color:#1a7a1a;");
    }

    // Stats
    if (m_ssmHistory.size() >= 2) {
        double sum = 0;
        for (double v : m_ssmHistory) sum += v;
        m_lblSsmMean->setText(QString("%1\"").arg(sum / m_ssmHistory.size(), 0, 'f', 2));
    }

    // Trigger logic (only when input level is valid)
    if (m_chkSsmTrigger->isChecked() && inputLevel >= 0.5) {
        if (seeing <= m_spinSsmThreshold->value()) {
            ++m_ssmConsecutive;
            if (m_ssmConsecutive == m_spinSsmConsec->value()) {
                ++m_ssmTriggerCount;
                QString ts = QDateTime::currentDateTime().toString("HH:mm:ss");
                QString msg = QString("Triggered #%1  %2\"  @ %3")
                              .arg(m_ssmTriggerCount).arg(seeing, 0, 'f', 2).arg(ts);
                m_lblSsmTrigger->setText(msg);
                m_lblSsmTrigger->setStyleSheet("color:#1a7a1a; font-weight:bold; font-size:10px;");
                statusBar()->showMessage("SSM " + msg);
                // Auto-start recording
                if (m_grabber && !m_btnRecord->isChecked()) {
                    m_btnRecord->setChecked(true);
                    onRecordToggled(true);
                }
            }
        } else {
            m_ssmConsecutive = 0;
            m_lblSsmTrigger->setText("Trigger: waiting for good seeing…");
            m_lblSsmTrigger->setStyleSheet("color:#b35c00; font-size:10px;");
        }
    }
}

void MainWindow::onSsmRawLine(const QString &token)
{
    if (m_txtSsmRaw->isVisible())
        m_txtSsmRaw->appendPlainText(token);
}

// ── buildUi ───────────────────────────────────────────────────────────────────

void MainWindow::buildUi()
{
    auto *central = new QWidget(this);
    setCentralWidget(central);
    auto *root = new QHBoxLayout(central);
    root->setSpacing(6);
    root->setContentsMargins(6, 6, 6, 6);

    // ═══════════════════════════════════════════════════════════════════════
    // LEFT PANEL – camera parameters
    // ═══════════════════════════════════════════════════════════════════════
    auto *leftPanel = new QVBoxLayout();
    leftPanel->setSpacing(4);

    // Camera connect bar
    auto *camBar = new QHBoxLayout();
    m_comboCameraBrand = new QComboBox();
    m_comboCameraBrand->addItems({"Basler", "ZWO", "Player One"});
    m_comboCameraBrand->setFixedWidth(100);
    camBar->addWidget(m_comboCameraBrand);
    camBar->addSpacing(6);
    m_btnConnect = makeToggleBtn("Camera Connect");
    m_btnConnect->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    camBar->addWidget(m_btnConnect);
    leftPanel->addLayout(camBar);

    // Scrollable camera controls
    auto *leftScroll = new QScrollArea();
    leftScroll->setWidgetResizable(true);
    leftScroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    leftScroll->setFrameShape(QFrame::NoFrame);

    auto *leftInner    = new QWidget();
    auto *leftInnerLay = new QVBoxLayout(leftInner);
    leftInnerLay->setSpacing(4);
    leftInnerLay->setContentsMargins(0, 0, 4, 0);
    leftInnerLay->addWidget(makeCameraInfoGroup());
    leftInnerLay->addWidget(makeExposureGroup());
    leftInnerLay->addWidget(makeOffsetGroup());
    leftInnerLay->addWidget(makeHistogramGroup());
    leftInnerLay->addStretch();
    leftInnerLay->addWidget(makeActionButtons());
    leftScroll->setWidget(leftInner);
    leftPanel->addWidget(leftScroll);

    root->addLayout(leftPanel, 2);

    // ═══════════════════════════════════════════════════════════════════════
    // CENTRE PANEL – preview
    // ═══════════════════════════════════════════════════════════════════════
    auto *centre = new QVBoxLayout();
    centre->setSpacing(4);

    // Top toolbar
    auto *topBar = new QHBoxLayout();
    topBar->addWidget(new QLabel("Zoom:"));
    m_comboZoom = new QComboBox();
    const QStringList zoomLevels = {"Fit","25%","33%","50%","75%","100%",
                                     "150%","200%","300%","400%"};
    m_comboZoom->addItems(zoomLevels);
    m_comboZoom->setCurrentText("Fit");
    m_comboZoom->setFixedWidth(80);
    topBar->addWidget(m_comboZoom);
    topBar->addSpacing(16);
    topBar->addWidget(new QLabel("Palette:"));
    m_comboPalette = new QComboBox();
    m_comboPalette->addItems({"Original","Inverted","Rainbow","Red Hot","Cool"});
    m_comboPalette->setFixedWidth(90);
    topBar->addWidget(m_comboPalette);
    topBar->addSpacing(16);
    auto *btnPlayer = new QPushButton("▶  Play SER");
    btnPlayer->setFixedHeight(28);
    btnPlayer->setFixedWidth(95);
    btnPlayer->setStyleSheet("padding:2px 6px;");
    connect(btnPlayer, &QPushButton::clicked, this, [this] {
        if (!m_serPlayer) {
            m_serPlayer = new SerPlayerDialog(this);
            m_serPlayer->setAttribute(Qt::WA_DeleteOnClose);
            connect(m_serPlayer, &QObject::destroyed,
                    this, [this] { m_serPlayer = nullptr; });
        }
        m_serPlayer->show();
        m_serPlayer->raise();
        m_serPlayer->activateWindow();
    });
    topBar->addWidget(btnPlayer);
    // Live view starts automatically on connect — kept for internal state only
    m_btnLive = new QPushButton();
    m_btnLive->setVisible(false);
    m_btnLive->setCheckable(true);
    topBar->addStretch();
    centre->addLayout(topBar);

    // Preview scroll area (camera image goes here in Phase 2)
    m_scrollArea = new QScrollArea();
    m_scrollArea->setAlignment(Qt::AlignCenter);
    m_scrollArea->setStyleSheet("background:#1c1c1c; border:1px solid #bbb;");
    m_scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    m_scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    m_previewLabel = new PreviewWidget();
    m_previewLabel->setAlignment(Qt::AlignCenter);
    m_previewLabel->setStyleSheet("color:#aaa; font-size:18px;");
    m_previewLabel->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);
    m_previewLabel->setText("No camera connected");
    m_scrollArea->setWidget(m_previewLabel);
    centre->addWidget(m_scrollArea);

    root->addLayout(centre, 5);

    // ═══════════════════════════════════════════════════════════════════════
    // RIGHT PANEL – SSM
    // ═══════════════════════════════════════════════════════════════════════
    auto *rightPanel = new QVBoxLayout();
    rightPanel->setSpacing(4);

    // SSM connection row
    auto *ssmBar = new QHBoxLayout();
    ssmBar->addWidget(new QLabel("Port:"));
    m_comboSsmPort = new QComboBox();
    m_comboSsmPort->setFixedWidth(95);
    ssmBar->addWidget(m_comboSsmPort);
    ssmBar->addSpacing(4);
    ssmBar->addWidget(new QLabel("Baud:"));
    m_comboSsmBaud = new QComboBox();
    m_comboSsmBaud->addItems({"9600","19200","38400","57600","115200"});
    m_comboSsmBaud->setCurrentText("115200");
    m_comboSsmBaud->setFixedWidth(75);
    ssmBar->addWidget(m_comboSsmBaud);
    ssmBar->addSpacing(4);
    m_btnSsm = makeToggleBtn("SSM Connect");
    ssmBar->addWidget(m_btnSsm);
    ssmBar->addStretch();
    rightPanel->addLayout(ssmBar);

    rightPanel->addWidget(makeSsmGroup());
    rightPanel->addWidget(makeSamplingGroup());
    rightPanel->addStretch();

    root->addLayout(rightPanel, 1);

    // ── Status bar ────────────────────────────────────────────────────────
    auto *sb = new QStatusBar(this);
    setStatusBar(sb);

    m_lblFps = new QLabel("fps: —");
    m_lblFps->setStyleSheet(
        "color:#1a7a1a; font-weight:bold; padding:0 10px; border-left:1px solid #aaa;");
    m_lblFps->setMinimumWidth(80);

    m_lblMbps = new QLabel("MB/s: —");
    m_lblMbps->setStyleSheet(
        "color:#1a7a1a; font-weight:bold; padding:0 10px; border-left:1px solid #aaa;");
    m_lblMbps->setMinimumWidth(90);

    sb->addPermanentWidget(m_lblFps);
    sb->addPermanentWidget(m_lblMbps);
    sb->showMessage("Ready – Camera not connected");
}

// ── Left panel group factories ────────────────────────────────────────────────

QGroupBox *MainWindow::makeCameraInfoGroup()
{
    auto *grp = new QGroupBox("Camera");
    auto *lay = new QVBoxLayout(grp);
    m_lblCameraInfo = new QLabel("Not connected");
    m_lblCameraInfo->setWordWrap(true);
    lay->addWidget(m_lblCameraInfo);
    return grp;
}

QGroupBox *MainWindow::makeExposureGroup()
{
    auto *grp = new QGroupBox("Acquisition");
    auto *lay = new QGridLayout(grp);
    lay->setVerticalSpacing(4);
    lay->setHorizontalSpacing(4);

    // ── Exposure row ─────────────────────────────────────────────────────────
    lay->addWidget(new QLabel("Exp (µs):"), 0, 0);
    m_spinExposure = new QDoubleSpinBox();
    m_spinExposure->setRange(100, 1'000'000);
    m_spinExposure->setSingleStep(100);
    m_spinExposure->setValue(10000);
    m_spinExposure->setDecimals(1);
    m_spinExposure->setFixedWidth(90);
    lay->addWidget(m_spinExposure, 0, 1);
    m_sliderExposure = new QSlider(Qt::Horizontal);
    m_sliderExposure->setRange(1, 1000);
    m_sliderExposure->setValue(static_cast<int>(1.0 + 999.0 * std::log10(10000.0 / 100.0) / 4.0));
    lay->addWidget(m_sliderExposure, 0, 2);

    // ── Gain row ─────────────────────────────────────────────────────────────
    lay->addWidget(new QLabel("Gain (dB):"), 1, 0);
    m_spinGain = new QDoubleSpinBox();
    m_spinGain->setRange(0, 24);
    m_spinGain->setSingleStep(0.1);
    m_spinGain->setDecimals(2);
    m_spinGain->setFixedWidth(90);
    lay->addWidget(m_spinGain, 1, 1);
    m_sliderGain = new QSlider(Qt::Horizontal);
    m_sliderGain->setRange(0, 240);
    lay->addWidget(m_sliderGain, 1, 2);

    // ── Pixel format row ─────────────────────────────────────────────────────
    lay->addWidget(new QLabel("Format:"), 2, 0);
    m_comboFormat = new QComboBox();
    m_comboFormat->addItems({"Mono8", "Mono12", "BayerRG8"});
    lay->addWidget(m_comboFormat, 2, 1, 1, 2);

    // ── Separator ─────────────────────────────────────────────────────────────
    auto *sep = new QFrame();
    sep->setFrameShape(QFrame::HLine);
    sep->setStyleSheet("color:#bbb;");
    lay->addWidget(sep, 3, 0, 1, 3);

    // ── Frames row ───────────────────────────────────────────────────────────
    lay->addWidget(new QLabel("Frames:"), 4, 0);
    m_spinFrames = new QSpinBox();
    m_spinFrames->setRange(1, 100000);
    m_spinFrames->setValue(100);
    lay->addWidget(m_spinFrames, 4, 1, 1, 2);

    // ── File name row ─────────────────────────────────────────────────────────
    lay->addWidget(new QLabel("File name:"), 5, 0);
    m_txtFilename = new QLineEdit();
    m_txtFilename->setPlaceholderText("Leave empty for auto timestamp");
    lay->addWidget(m_txtFilename, 5, 1, 1, 2);

    // ── Directory row ─────────────────────────────────────────────────────────
    lay->addWidget(new QLabel("Directory:"), 6, 0);
    m_btnSaveDir = new QPushButton("Browse…");
    lay->addWidget(m_btnSaveDir, 6, 1, 1, 2);

    m_lblSaveDir = new QLabel(QDir::homePath());
    m_lblSaveDir->setStyleSheet("color:#555; font-size:11px;");
    m_lblSaveDir->setWordWrap(true);
    lay->addWidget(m_lblSaveDir, 7, 0, 1, 3);

    // ── Record + Snap buttons ─────────────────────────────────────────────────
    auto *recRow = new QHBoxLayout();
    m_btnRecord = makeToggleBtn("⏺  Record", 110);
    recRow->addWidget(m_btnRecord);
    auto *btnSnap = new QPushButton("📷  Snap");
    btnSnap->setFixedHeight(28);
    btnSnap->setFixedWidth(80);
    btnSnap->setStyleSheet("padding:2px 6px;");
    connect(btnSnap, &QPushButton::clicked, this, &MainWindow::onSnapFrame);
    recRow->addWidget(btnSnap);
    recRow->addStretch();
    lay->addLayout(recRow, 8, 0, 1, 3);

    m_progressBar = new QProgressBar();
    m_progressBar->setVisible(false);
    lay->addWidget(m_progressBar, 9, 0, 1, 3);

    lay->setColumnStretch(2, 1);
    return grp;
}

QGroupBox *MainWindow::makeOffsetGroup()
{
    auto *grp  = new QGroupBox("ROI / Offset");
    auto *outer = new QVBoxLayout(grp);
    outer->setSpacing(4);

    auto *grid = new QGridLayout();
    grid->addWidget(new QLabel("Offset X:"), 0, 0);
    m_spinOffsetX = new QSpinBox();
    m_spinOffsetX->setRange(0, 1900);
    m_spinOffsetX->setSingleStep(4);
    grid->addWidget(m_spinOffsetX, 0, 1);
    grid->addWidget(new QLabel("Offset Y:"), 0, 2);
    m_spinOffsetY = new QSpinBox();
    m_spinOffsetY->setRange(0, 1190);
    m_spinOffsetY->setSingleStep(4);
    grid->addWidget(m_spinOffsetY, 0, 3);

    grid->addWidget(new QLabel("Width:"), 1, 0);
    m_spinWidth = new QSpinBox();
    m_spinWidth->setRange(64, 1920);
    m_spinWidth->setSingleStep(4);
    m_spinWidth->setValue(1920);
    grid->addWidget(m_spinWidth, 1, 1);
    grid->addWidget(new QLabel("Height:"), 1, 2);
    m_spinHeight = new QSpinBox();
    m_spinHeight->setRange(64, 1200);
    m_spinHeight->setSingleStep(4);
    m_spinHeight->setValue(1200);
    grid->addWidget(m_spinHeight, 1, 3);
    outer->addLayout(grid);

    m_btnClearRoi = new QPushButton("✕  Clear ROI – full sensor");
    m_btnClearRoi->setMinimumHeight(32);
    m_btnClearRoi->setStyleSheet(Theme::warnButtonStyle());
    m_btnClearRoi->setEnabled(false);
    outer->addWidget(m_btnClearRoi);

    auto *hint = new QLabel("Click and drag on the image to draw a ROI");
    hint->setStyleSheet("color:#555; font-size:10px; font-style:italic;");
    hint->setWordWrap(true);
    outer->addWidget(hint);
    return grp;
}

QGroupBox *MainWindow::makeHistogramGroup()
{
    auto *grp = new QGroupBox("Histogram");
    auto *lay = new QVBoxLayout(grp);
    lay->setSpacing(4);

    // Real histogram widget
    m_histogramWidget = new HistogramWidget();
    lay->addWidget(m_histogramWidget);

    m_chkHistLog = new QCheckBox("Log scale (Y axis)");
    lay->addWidget(m_chkHistLog);

    // Display levels
    auto *lvl = new QGridLayout();
    lvl->setVerticalSpacing(2);
    lvl->addWidget(new QLabel("Black:"), 0, 0);
    m_sliderBlack = new QSlider(Qt::Horizontal);
    m_sliderBlack->setRange(0, 254);
    m_sliderBlack->setValue(0);
    lvl->addWidget(m_sliderBlack, 0, 1);
    m_lblBlack = new QLabel("0");
    m_lblBlack->setFixedWidth(28);
    lvl->addWidget(m_lblBlack, 0, 2);

    lvl->addWidget(new QLabel("White:"), 1, 0);
    m_sliderWhite = new QSlider(Qt::Horizontal);
    m_sliderWhite->setRange(1, 255);
    m_sliderWhite->setValue(255);
    lvl->addWidget(m_sliderWhite, 1, 1);
    m_lblWhite = new QLabel("255");
    m_lblWhite->setFixedWidth(28);
    lvl->addWidget(m_lblWhite, 1, 2);
    lay->addLayout(lvl);

    auto *note = new QLabel("Display only – does not affect recorded data");
    note->setStyleSheet("color:#555; font-size:10px; font-style:italic;");
    lay->addWidget(note);
    return grp;
}

QWidget *MainWindow::makeActionButtons()
{
    auto *w   = new QWidget();
    auto *lay = new QVBoxLayout(w);
    lay->setSpacing(6);
    // placeholder for future action buttons
    return w;
}

QGroupBox *MainWindow::makeSamplingGroup()
{
    auto *grp = new QGroupBox("Sampling Calculator");
    auto *lay = new QGridLayout(grp);
    lay->setVerticalSpacing(4);
    lay->setHorizontalSpacing(4);

    auto makeSpin = [](double min, double max, double val,
                       double step, int decimals, const QString &suffix) {
        auto *s = new QDoubleSpinBox();
        s->setRange(min, max);
        s->setValue(val);
        s->setSingleStep(step);
        s->setDecimals(decimals);
        s->setSuffix(suffix);
        s->setFixedWidth(95);
        return s;
    };

    // Row 0: D | F
    lay->addWidget(new QLabel("D:"),           0, 0);
    m_spinDiameter    = makeSpin(10, 2000, 200, 10, 0, " mm");
    lay->addWidget(m_spinDiameter,             0, 1);
    lay->addWidget(new QLabel("F:"),           0, 2);
    m_spinFocalLength = makeSpin(100, 20000, 3000, 100, 0, " mm");
    lay->addWidget(m_spinFocalLength,          0, 3);

    // Row 1: PixSize | λ
    lay->addWidget(new QLabel("Pix:"),         1, 0);
    m_spinPixelSize   = makeSpin(1.0, 30.0, 2.9, 0.1, 2, " µm");
    lay->addWidget(m_spinPixelSize,            1, 1);
    lay->addWidget(new QLabel(u8"\u03BB:"),    1, 2);
    m_spinWavelength  = makeSpin(300, 1100, 550, 10, 0, " nm");
    lay->addWidget(m_spinWavelength,           1, 3);

    // Results
    auto *sep = new QFrame();
    sep->setFrameShape(QFrame::HLine);
    sep->setStyleSheet("color:#bbb;");
    lay->addWidget(sep, 2, 0, 1, 4);

    lay->addWidget(new QLabel("Sampling:"),   3, 0, 1, 2);
    m_lblSampling = new QLabel("—");
    m_lblSampling->setStyleSheet("font-weight:bold;");
    lay->addWidget(m_lblSampling,             3, 2, 1, 2);

    lay->addWidget(new QLabel("S/N factor:"), 4, 0, 1, 2);
    m_lblSamplingFactor = new QLabel("—");
    m_lblSamplingFactor->setStyleSheet("font-weight:bold;");
    lay->addWidget(m_lblSamplingFactor,       4, 2, 1, 2);

    auto *hint = new QLabel("Shannon-Nyquist: >3 good, 2–3 ok, <2 undersampled");
    hint->setStyleSheet("color:#555; font-size:9px; font-style:italic;");
    hint->setWordWrap(true);
    lay->addWidget(hint, 5, 0, 1, 4);

    auto calc = [this]() {
        const double D   = m_spinDiameter->value();
        const double F   = m_spinFocalLength->value();
        const double pix = m_spinPixelSize->value();
        const double lam = m_spinWavelength->value();

        const double pixMm      = pix / 1000.0;
        const double sampling   = 206265.0 * pixMm / F;
        const double lamMm      = lam / 1e6;
        const double resolution = 1.22 * lamMm / D * 206265.0;
        const double factor     = resolution / sampling;

        m_lblSampling->setText(QString("%1 \"/px").arg(sampling, 0, 'f', 3));

        QString col;
        if      (factor >= 3.0) col = "#1a7a1a";
        else if (factor >= 2.0) col = "#b35c00";
        else                    col = "#c0392b";

        m_lblSamplingFactor->setText(QString("%1").arg(factor, 0, 'f', 2));
        m_lblSamplingFactor->setStyleSheet(
            QString("font-weight:bold; color:%1;").arg(col));
    };

    auto recalc = [calc](double) { calc(); };
    connect(m_spinDiameter,    QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, recalc);
    connect(m_spinFocalLength, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, recalc);
    connect(m_spinPixelSize,   QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, recalc);
    connect(m_spinWavelength,  QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, recalc);

    calc();
    return grp;
}

// ── SSM group ─────────────────────────────────────────────────────────────────

QGroupBox *MainWindow::makeSsmGroup()
{
    auto *grp  = new QGroupBox("Seeing");
    auto *outer = new QVBoxLayout(grp);
    outer->setSpacing(4);

    // Stats grid
    auto *statsGrid = new QGridLayout();
    statsGrid->setVerticalSpacing(2);

    statsGrid->addWidget(new QLabel("Seeing (index):"), 0, 0);
    m_lblSsmCurrent = new QLabel("—");
    m_lblSsmCurrent->setStyleSheet("font-weight:bold; font-size:15px; color:#555;");
    statsGrid->addWidget(m_lblSsmCurrent, 0, 1);

    auto *lblInp = new QLabel("Input level:");
    lblInp->setToolTip("Photodiode illumination (0.5–1.0 normal, <0.5 = seeing invalid)");
    statsGrid->addWidget(lblInp, 1, 0);
    m_lblSsmInput = new QLabel("—");
    m_lblSsmInput->setStyleSheet("font-weight:bold; font-size:13px;");
    statsGrid->addWidget(m_lblSsmInput, 1, 1);

    statsGrid->addWidget(new QLabel("Mean:"), 0, 2);
    m_lblSsmMean = new QLabel("—");
    m_lblSsmMean->setStyleSheet("font-weight:bold;");
    statsGrid->addWidget(m_lblSsmMean, 0, 3);
    outer->addLayout(statsGrid);

    // Plot placeholder — replaced by SeePlot in Phase 6
    m_ssmPlot = new SeePlot();
    outer->addWidget(m_ssmPlot);

    // Trigger controls
    auto *trigRow = new QHBoxLayout();
    trigRow->addWidget(new QLabel("Threshold:"));
    m_spinSsmThreshold = new QDoubleSpinBox();
    m_spinSsmThreshold->setRange(0.1, 10.0);
    m_spinSsmThreshold->setSingleStep(0.1);
    m_spinSsmThreshold->setDecimals(2);
    m_spinSsmThreshold->setValue(2.0);
    m_spinSsmThreshold->setSuffix(" \"");
    m_spinSsmThreshold->setFixedWidth(80);
    trigRow->addWidget(m_spinSsmThreshold);
    trigRow->addSpacing(8);
    trigRow->addWidget(new QLabel("Consec:"));
    m_spinSsmConsec = new QSpinBox();
    m_spinSsmConsec->setRange(1, 100);
    m_spinSsmConsec->setValue(3);
    m_spinSsmConsec->setFixedWidth(55);
    m_spinSsmConsec->setToolTip(
        "Consecutive samples below threshold before trigger fires");
    trigRow->addWidget(m_spinSsmConsec);
    trigRow->addStretch();
    outer->addLayout(trigRow);

    m_chkSsmTrigger = new QCheckBox("Auto-trigger recording");
    m_chkSsmTrigger->setToolTip(
        "When seeing stays below threshold for N consecutive samples,\n"
        "automatically start a camera recording.");
    outer->addWidget(m_chkSsmTrigger);

    m_lblSsmTrigger = new QLabel("Trigger: idle");
    m_lblSsmTrigger->setStyleSheet("color:#555; font-size:10px;");
    outer->addWidget(m_lblSsmTrigger);

    // Logging row
    auto *logRow = new QHBoxLayout();
    m_btnSsmLog = new QPushButton("Log SSM CSV");
    m_btnSsmLog->setCheckable(true);
    m_btnSsmLog->setFixedHeight(24);
    m_btnSsmLog->setStyleSheet(
        "QPushButton { background:#c0392b; color:white; font-weight:bold;"
        " border-radius:4px; padding:1px 6px; font-size:11px; }"
        "QPushButton:checked { background:#27ae60; }");
    logRow->addWidget(m_btnSsmLog);
    m_lblSsmLogPath = new QLabel("No file");
    m_lblSsmLogPath->setStyleSheet("color:#555; font-size:10px;");
    m_lblSsmLogPath->setWordWrap(true);
    logRow->addWidget(m_lblSsmLogPath, 1);
    outer->addLayout(logRow);

    // Raw serial monitor
    m_chkSsmRaw = new QCheckBox("Show raw serial data");
    m_chkSsmRaw->setStyleSheet("font-size:10px;");
    outer->addWidget(m_chkSsmRaw);

    m_txtSsmRaw = new QPlainTextEdit();
    m_txtSsmRaw->setReadOnly(true);
    m_txtSsmRaw->setMaximumBlockCount(50);
    m_txtSsmRaw->setFixedHeight(80);
    m_txtSsmRaw->setStyleSheet(
        "font-family:monospace; font-size:10px;"
        "background:#f8f8f8; color:#333;");
    m_txtSsmRaw->setVisible(false);
    connect(m_chkSsmRaw, &QCheckBox::stateChanged, this,
            [this](int s){ m_txtSsmRaw->setVisible(s == Qt::Checked); });
    outer->addWidget(m_txtSsmRaw);

    return grp;
}
