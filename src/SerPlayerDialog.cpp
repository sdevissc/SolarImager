#include "SerPlayerDialog.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QSlider>
#include <QPushButton>
#include <QComboBox>
#include <QFileDialog>
#include <QKeyEvent>
#include <QTimer>
#include <QScrollArea>
#include <QMessageBox>
#include <QFileInfo>
#include <QDebug>

#include <cstring>
#include <algorithm>

// ── SER header (packed, 178 bytes) ────────────────────────────────────────────
#pragma pack(push, 1)
struct SerHdr {
    char    fileID[14];
    int32_t luID;
    int32_t colorID;
    int32_t littleEndian;
    int32_t width;
    int32_t height;
    int32_t pixelDepth;
    int32_t frameCount;
    char    observer[40];
    char    instrument[40];
    char    telescope[40];
    int64_t dateTime;
    int64_t dateTimeUTC;
};
#pragma pack(pop)
static_assert(sizeof(SerHdr) == 178, "SER header must be 178 bytes");

// ── Constructor ───────────────────────────────────────────────────────────────

SerPlayerDialog::SerPlayerDialog(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle("SER Player");
    setMinimumSize(700, 560);
    resize(780, 600);

    // ── Top bar ───────────────────────────────────────────────────────────────
    auto *topBar = new QHBoxLayout();
    m_btnOpen = new QPushButton("📂  Open SER…");
    m_btnOpen->setFixedHeight(28);
    topBar->addWidget(m_btnOpen);
    topBar->addSpacing(8);
    m_infoLabel = new QLabel("No file loaded");
    m_infoLabel->setStyleSheet("color:#555; font-size:11px;");
    topBar->addWidget(m_infoLabel, 1);

    // ── Image area ────────────────────────────────────────────────────────────
    m_imageLabel = new QLabel();
    m_imageLabel->setAlignment(Qt::AlignCenter);
    m_imageLabel->setStyleSheet("background:#111;");
    m_imageLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    m_imageLabel->setMinimumHeight(400);

    // ── Frame slider row ──────────────────────────────────────────────────────
    auto *frameRow = new QHBoxLayout();
    frameRow->addWidget(new QLabel("Frame:"));
    m_frameSlider = new QSlider(Qt::Horizontal);
    m_frameSlider->setRange(0, 0);
    m_frameSlider->setValue(0);
    frameRow->addWidget(m_frameSlider, 1);
    m_frameLabel = new QLabel("0 / 0");
    m_frameLabel->setFixedWidth(80);
    m_frameLabel->setStyleSheet("font-family:monospace;");
    frameRow->addWidget(m_frameLabel);

    // ── Level slider row ──────────────────────────────────────────────────────
    auto *levelRow = new QHBoxLayout();
    levelRow->addWidget(new QLabel("Level %:"));
    m_levelSlider = new QSlider(Qt::Horizontal);
    m_levelSlider->setRange(1, 100);
    m_levelSlider->setValue(100);
    levelRow->addWidget(m_levelSlider, 1);
    m_levelLabel = new QLabel("100%");
    m_levelLabel->setFixedWidth(40);
    m_levelLabel->setStyleSheet("font-family:monospace;");
    levelRow->addWidget(m_levelLabel);

    // ── Playback controls ─────────────────────────────────────────────────────
    auto *ctrlRow = new QHBoxLayout();
    m_btnPlay = new QPushButton("▶  Play");
    m_btnPlay->setFixedHeight(28);
    m_btnPlay->setFixedWidth(90);
    m_btnPlay->setEnabled(false);
    ctrlRow->addWidget(m_btnPlay);
    ctrlRow->addSpacing(8);
    ctrlRow->addWidget(new QLabel("FPS:"));
    m_comboFps = new QComboBox();
    m_comboFps->addItems({"5","10","15","25","30","50"});
    m_comboFps->setCurrentText("25");
    m_comboFps->setFixedWidth(55);
    ctrlRow->addWidget(m_comboFps);
    ctrlRow->addStretch();

    // Keyboard hint
    auto *hint = new QLabel("← → step frame   Space play/pause   Esc close");
    hint->setStyleSheet("color:#888; font-size:10px;");
    ctrlRow->addWidget(hint);

    // ── Main layout ───────────────────────────────────────────────────────────
    auto *main = new QVBoxLayout(this);
    main->setSpacing(6);
    main->setContentsMargins(8, 8, 8, 8);
    main->addLayout(topBar);
    main->addWidget(m_imageLabel, 1);
    main->addLayout(frameRow);
    main->addLayout(levelRow);
    main->addLayout(ctrlRow);

    // ── Timer ─────────────────────────────────────────────────────────────────
    m_timer = new QTimer(this);
    connect(m_timer, &QTimer::timeout, this, &SerPlayerDialog::onNextFrame);

    // ── Connections ───────────────────────────────────────────────────────────
    connect(m_btnOpen,    &QPushButton::clicked,
            this, &SerPlayerDialog::onOpenFile);
    connect(m_btnPlay,    &QPushButton::clicked,
            this, &SerPlayerDialog::onPlayPause);
    connect(m_frameSlider, &QSlider::valueChanged,
            this, &SerPlayerDialog::onFrameSlider);
    connect(m_levelSlider, &QSlider::valueChanged,
            this, [this](int v) {
        m_levelLabel->setText(QString("%1%").arg(v));
        showFrame(m_currentFrame);
    });
    connect(m_comboFps, &QComboBox::currentTextChanged,
            this, [this](const QString &s) {
        if (m_timer->isActive()) {
            m_timer->setInterval(1000 / std::max(1, s.toInt()));
        }
    });
}

SerPlayerDialog::~SerPlayerDialog()
{
    m_timer->stop();
    if (m_file.isOpen()) m_file.close();
}

// ── File loading ──────────────────────────────────────────────────────────────

void SerPlayerDialog::onOpenFile()
{
    QString path = QFileDialog::getOpenFileName(
        this, "Open SER file", QString(),
        "SER files (*.ser);;All files (*.*)");
    if (!path.isEmpty())
        openFile(path);
}

void SerPlayerDialog::openFile(const QString &path)
{
    m_timer->stop();
    m_btnPlay->setText("▶  Play");
    if (m_file.isOpen()) m_file.close();

    if (!loadHeader(path)) return;

    m_file.setFileName(path);
    if (!m_file.open(QIODevice::ReadOnly)) {
        QMessageBox::warning(this, "SER Player",
                             "Cannot open file: " + path);
        return;
    }

    m_currentFrame = 0;
    m_frameSlider->setRange(0, m_frameCount - 1);
    m_frameSlider->setValue(0);
    m_btnPlay->setEnabled(m_frameCount > 1);
    updateInfo();
    showFrame(0);
}

bool SerPlayerDialog::loadHeader(const QString &path)
{
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly)) return false;
    QByteArray raw = f.read(178);
    f.close();
    if (raw.size() < 178) return false;

    const SerHdr *h = reinterpret_cast<const SerHdr*>(raw.constData());

    // Validate magic
    if (strncmp(h->fileID, "LUCAM-RECORDER", 14) != 0) {
        QMessageBox::warning(this, "SER Player",
                             "Not a valid SER file (bad magic bytes).");
        return false;
    }

    m_width        = h->width;
    m_height       = h->height;
    m_pixelDepth   = h->pixelDepth;
    m_colorId      = h->colorID;
    m_littleEndian = (h->littleEndian != 0);
    m_frameCount   = h->frameCount;
    m_dataOffset   = 178;

    int bps    = (m_pixelDepth > 8) ? 2 : 1;
    int planes = (m_colorId == 100 || m_colorId == 101) ? 3 : 1;
    m_bytesPerFrame = m_width * m_height * planes * bps;

    // Sanity check against actual file size
    QFileInfo fi(path);
    qint64 available = (fi.size() - 178) / m_bytesPerFrame;
    if (available < m_frameCount) {
        qWarning() << "[SerPlayer] File truncated:"
                   << available << "of" << m_frameCount << "frames available";
        m_frameCount = static_cast<int>(available);
    }

    return m_frameCount > 0;
}

// ── Frame reading ─────────────────────────────────────────────────────────────

QImage SerPlayerDialog::readFrame(int index)
{
    if (!m_file.isOpen() || index < 0 || index >= m_frameCount)
        return {};

    qint64 offset = m_dataOffset + static_cast<qint64>(index) * m_bytesPerFrame;
    if (!m_file.seek(offset)) return {};

    QByteArray buf = m_file.read(m_bytesPerFrame);
    if (buf.size() < m_bytesPerFrame) return {};

    const int   level  = m_levelSlider->value();   // 1–100
    const float wpoint = level / 100.0f;

    // 16-bit mono
    if (m_pixelDepth > 8 && m_colorId == 0) {
        const uint16_t *src = reinterpret_cast<const uint16_t*>(buf.constData());
        QImage img(m_width, m_height, QImage::Format_Grayscale8);
        for (int y = 0; y < m_height; ++y) {
            uchar *dst = img.scanLine(y);
            for (int x = 0; x < m_width; ++x) {
                float v = src[y * m_width + x] / 65535.0f / wpoint;
                dst[x] = static_cast<uchar>(std::clamp(v * 255.0f, 0.f, 255.f));
            }
        }
        return img;
    }

    // 8-bit mono
    if (m_pixelDepth <= 8 && m_colorId == 0) {
        QImage img(m_width, m_height, QImage::Format_Grayscale8);
        for (int y = 0; y < m_height; ++y) {
            const uchar *src = reinterpret_cast<const uchar*>(buf.constData())
                               + y * m_width;
            uchar *dst = img.scanLine(y);
            if (wpoint >= 1.0f) {
                std::memcpy(dst, src, m_width);
            } else {
                for (int x = 0; x < m_width; ++x) {
                    float v = src[x] / 255.0f / wpoint;
                    dst[x] = static_cast<uchar>(std::clamp(v * 255.0f, 0.f, 255.f));
                }
            }
        }
        return img;
    }

    // RGB (colorId 100)
    if (m_colorId == 100) {
        QImage img(m_width, m_height, QImage::Format_RGB888);
        for (int y = 0; y < m_height; ++y) {
            const uchar *src = reinterpret_cast<const uchar*>(buf.constData())
                               + y * m_width * 3;
            uchar *dst = img.scanLine(y);
            if (wpoint >= 1.0f) {
                std::memcpy(dst, src, m_width * 3);
            } else {
                for (int x = 0; x < m_width * 3; ++x) {
                    float v = src[x] / 255.0f / wpoint;
                    dst[x] = static_cast<uchar>(std::clamp(v * 255.0f, 0.f, 255.f));
                }
            }
        }
        return img;
    }

    return {};
}

// ── Display ───────────────────────────────────────────────────────────────────

void SerPlayerDialog::showFrame(int index)
{
    QImage img = readFrame(index);
    if (img.isNull()) return;

    QPixmap pix = QPixmap::fromImage(img).scaled(
        m_imageLabel->size(), Qt::KeepAspectRatio, Qt::FastTransformation);
    m_imageLabel->setPixmap(pix);
    m_frameLabel->setText(QString("%1 / %2").arg(index).arg(m_frameCount - 1));
}

void SerPlayerDialog::updateInfo()
{
    const QString colorName = (m_colorId == 0) ? "MONO" :
                              (m_colorId == 100) ? "RGB" : QString("ID=%1").arg(m_colorId);
    m_infoLabel->setText(
        QString("%1   %2×%3   %4 frames   %5-bit   %6")
            .arg(QFileInfo(m_file.fileName()).fileName())
            .arg(m_width).arg(m_height)
            .arg(m_frameCount)
            .arg(m_pixelDepth)
            .arg(colorName));
}

// ── Playback ──────────────────────────────────────────────────────────────────

void SerPlayerDialog::onPlayPause()
{
    if (m_timer->isActive()) {
        m_timer->stop();
        m_btnPlay->setText("▶  Play");
    } else {
        int fps = m_comboFps->currentText().toInt();
        m_timer->start(1000 / std::max(1, fps));
        m_btnPlay->setText("⏸  Pause");
    }
}

void SerPlayerDialog::onNextFrame()
{
    int next = (m_currentFrame + 1) % m_frameCount;
    m_frameSlider->setValue(next);   // triggers onFrameSlider
}

void SerPlayerDialog::onFrameSlider(int value)
{
    m_currentFrame = value;
    showFrame(value);
}

// ── Keyboard ──────────────────────────────────────────────────────────────────

void SerPlayerDialog::keyPressEvent(QKeyEvent *e)
{
    switch (e->key()) {
    case Qt::Key_Right:
        m_timer->stop();
        m_btnPlay->setText("▶  Play");
        m_frameSlider->setValue(std::min(m_currentFrame + 1, m_frameCount - 1));
        break;
    case Qt::Key_Left:
        m_timer->stop();
        m_btnPlay->setText("▶  Play");
        m_frameSlider->setValue(std::max(m_currentFrame - 1, 0));
        break;
    case Qt::Key_Space:
        onPlayPause();
        break;
    case Qt::Key_Escape:
        close();
        break;
    default:
        QDialog::keyPressEvent(e);
    }
}
