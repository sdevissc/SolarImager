#pragma once

#include <QMainWindow>
#include <QVector>
#include <QStringList>
#include <QFile>
#include <QTextStream>

class QScrollArea;
class QLabel;
class PreviewWidget;
class QPushButton;
class QComboBox;
class QStatusBar;
class QProgressBar;
class QGroupBox;
class QDoubleSpinBox;
class QSpinBox;
class QCheckBox;
class QSlider;
class QLineEdit;
class QPlainTextEdit;
class QTimer;
class CameraInterface;
class FrameGrabber;
class HistogramWidget;
class SSMReader;
class SeePlot;
class SerPlayerDialog;

/// Main application window.
/// Phase 1: empty shell with correct 3-column layout and status bar.
/// Subsequent phases will populate each panel with real widgets.
class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

protected:
    void closeEvent(QCloseEvent *event) override;

private:
    // ── UI construction ──────────────────────────────────────────────────────
    void buildUi();

    // ── Left panel (camera controls) ────────────────────────────────────────
    QGroupBox *makeCameraInfoGroup();
    QGroupBox *makeExposureGroup();   // merged: Exposure + Gain + Pixel Format
    QGroupBox *makeOffsetGroup();
    QGroupBox *makeRecordingGroup();
    QGroupBox *makeHistogramGroup();
    QWidget   *makeActionButtons();

    // ── Centre panel (preview) ───────────────────────────────────────────────
    // (populated in Phase 2)

    // ── Right panel (SSM) ────────────────────────────────────────────────────
    QGroupBox *makeSsmGroup();

    // ── Camera slots ─────────────────────────────────────────────────────────
    void onCameraConnectToggled(bool checked);
    void onLiveToggled(bool checked);
    void onRecordToggled(bool checked);
    void onSnapFrame();
    void onFrameReady(QImage img);
    void onRecordingStats(double fps, double mbps, int framesDone);
    void onRecordingFinished();
    void onCameraError(const QString &msg);

    // ── SSM slots ─────────────────────────────────────────────────────────────
    void refreshSsmPorts();
    void onSsmConnectToggled(bool checked);
    void onSsmNewSample(double inputLevel, double seeing);
    void onSsmRawLine(const QString &token);
    void onSsmError(const QString &msg);
    void onSsmConnected();
    void onSsmDisconnected();

    // ── Widgets ──────────────────────────────────────────────────────────────

    // Top-bar / centre
    QComboBox    *m_comboZoom      = nullptr;
    QComboBox    *m_comboPalette   = nullptr;
    QPushButton  *m_btnLive        = nullptr;
    QPushButton  *m_btnRecord      = nullptr;
    QScrollArea  *m_scrollArea     = nullptr;
    PreviewWidget *m_previewLabel  = nullptr;
    QProgressBar *m_progressBar    = nullptr;

    // Left panel
    QPushButton  *m_btnConnect     = nullptr;
    QComboBox    *m_comboCameraBrand = nullptr;
    QLabel       *m_lblCameraInfo  = nullptr;

    // Exposure
    QDoubleSpinBox *m_spinExposure = nullptr;
    QSlider        *m_sliderExposure = nullptr;
    QCheckBox      *m_chkAutoExposure = nullptr;

    // Gain
    QDoubleSpinBox *m_spinGain     = nullptr;
    QSlider        *m_sliderGain   = nullptr;

    // ROI / Offset
    QSpinBox *m_spinOffsetX  = nullptr;
    QSpinBox *m_spinOffsetY  = nullptr;
    QSpinBox *m_spinWidth    = nullptr;
    QSpinBox *m_spinHeight   = nullptr;
    QPushButton *m_btnClearRoi = nullptr;

    // Format
    QComboBox *m_comboFormat = nullptr;

    // Recording
    QSpinBox  *m_spinFrames   = nullptr;
    QLineEdit *m_txtFilename  = nullptr;
    QLabel    *m_lblSaveDir   = nullptr;
    QPushButton *m_btnSaveDir = nullptr;

    // Histogram
    HistogramWidget *m_histogramWidget = nullptr;
    QCheckBox       *m_chkHistLog      = nullptr;

    // Display levels
    QSlider *m_sliderBlack = nullptr;
    QSlider *m_sliderWhite = nullptr;
    QLabel  *m_lblBlack    = nullptr;
    QLabel  *m_lblWhite    = nullptr;

    // Status bar
    QLabel *m_lblFps  = nullptr;
    QLabel *m_lblMbps = nullptr;

    // SSM panel
    QComboBox   *m_comboSsmPort  = nullptr;
    QComboBox   *m_comboSsmBaud  = nullptr;
    QPushButton *m_btnSsm        = nullptr;
    QLabel      *m_lblSsmCurrent = nullptr;
    QLabel      *m_lblSsmInput   = nullptr;
    QLabel      *m_lblSsmMean    = nullptr;
    QLabel      *m_lblSsmBest    = nullptr;
    SeePlot     *m_ssmPlot       = nullptr;
    QDoubleSpinBox *m_spinSsmThreshold = nullptr;
    QSpinBox       *m_spinSsmConsec    = nullptr;
    QCheckBox      *m_chkSsmTrigger    = nullptr;
    QLabel         *m_lblSsmTrigger    = nullptr;
    QPushButton    *m_btnSsmLog        = nullptr;
    QLabel         *m_lblSsmLogPath    = nullptr;
    QCheckBox      *m_chkSsmRaw        = nullptr;
    QPlainTextEdit *m_txtSsmRaw        = nullptr;

    // ── Backend objects ───────────────────────────────────────────────────────
    CameraInterface *m_camera        = nullptr;
    FrameGrabber    *m_grabber       = nullptr;
    SSMReader       *m_ssmReader     = nullptr;
    QTimer          *m_ssmPortTimer  = nullptr;
    QVector<double>  m_ssmHistory;
    int              m_ssmTriggerCount = 0;
    int              m_ssmConsecutive  = 0;

    // SSM CSV logging
    QFile            m_ssmLogFile;
    QTextStream      m_ssmLogStream;

    // SER player
    SerPlayerDialog *m_serPlayer = nullptr;
};
