#pragma once

#include <QDialog>
#include <QFile>
#include <QString>

class QLabel;
class QSlider;
class QPushButton;
class QComboBox;
class QTimer;

/// Lightweight SER file player.
/// Streams frames on demand — no full file load into memory.
class SerPlayerDialog : public QDialog
{
    Q_OBJECT

public:
    explicit SerPlayerDialog(QWidget *parent = nullptr);
    ~SerPlayerDialog() override;

    /// Open a SER file directly (called from main window with last recording path)
    void openFile(const QString &path);
    /// Set the default directory for the open file dialog
    void setDefaultDirectory(const QString &dir);

protected:
    void keyPressEvent(QKeyEvent *e) override;

private slots:
    void onOpenFile();
    void onPlayPause();
    void onFrameSlider(int value);
    void onNextFrame();   // called by timer

private:
    bool     loadHeader(const QString &path);
    QImage   readFrame(int index);
    void     showFrame(int index);
    void     updateInfo();

    // ── File state ────────────────────────────────────────────────────────────
    QFile    m_file;
    QString  m_defaultDir;  ///< Default directory for open dialog
    int      m_frameCount  = 0;
    int      m_width       = 0;
    int      m_height      = 0;
    int      m_pixelDepth  = 8;
    int      m_colorId     = 0;
    bool     m_littleEndian = true;
    qint64   m_dataOffset  = 178;
    int      m_bytesPerFrame = 0;
    int      m_currentFrame  = 0;

    // ── UI ────────────────────────────────────────────────────────────────────
    QLabel      *m_imageLabel   = nullptr;
    QLabel      *m_infoLabel    = nullptr;
    QLabel      *m_frameLabel   = nullptr;
    QSlider     *m_frameSlider  = nullptr;
    QSlider     *m_levelSlider  = nullptr;
    QLabel      *m_levelLabel   = nullptr;
    QPushButton *m_btnOpen      = nullptr;
    QPushButton *m_btnPlay      = nullptr;
    QComboBox   *m_comboFps     = nullptr;
    QTimer      *m_timer        = nullptr;
};
