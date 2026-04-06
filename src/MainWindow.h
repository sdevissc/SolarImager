#pragma once

#include <QMainWindow>
#include <QVector>
#include <QStringList>
#include <QFile>
#include <QTextStream>
#include "AppSettings.h"

class QScrollArea;
class QScrollBar;
class QTabWidget;
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
    void applySettings();

    // ── Panel factories ──────────────────────────────────────────────────────
    QGroupBox *makeCameraInfoGroup();
    QGroupBox *makeExposureGroup();
    QGroupBox *makeOffsetGroup();
    QGroupBox *makeHistogramGroup();
    QGroupBox *makeSamplingGroup();
    QGroupBox *makeSsmGroup();
    QWidget   *makeActionButtons();

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

    // Preview toolbar
    QComboBox    *m_comboZoom       = nullptr;
    QComboBox    *m_comboPalette    = nullptr;
    QCheckBox    *m_btnSatHighlight = nullptr;
    QPushButton  *m_btnLive         = nullptr;  // hidden, internal state only
    QPushButton  *m_btnRecord       = nullptr;
    QScrollArea  *m_scrollArea      = nullptr;
    PreviewWidget *m_previewLabel   = nullptr;
    QProgressBar *m_progressBar     = nullptr;

    // Left panel — camera
    QPushButton  *m_btnConnect      = nullptr;
    QComboBox    *m_comboCameraBrand = nullptr;
    QLabel       *m_lblCameraInfo   = nullptr;

    // Exposure / Gain
    QDoubleSpinBox *m_spinExposure   = nullptr;
    QSlider        *m_sliderExposure = nullptr;
    QDoubleSpinBox *m_spinGain       = nullptr;
    QSlider        *m_sliderGain     = nullptr;

    // ROI / Offset
    QSpinBox    *m_spinOffsetX  = nullptr;
    QSpinBox    *m_spinOffsetY  = nullptr;
    QSpinBox    *m_spinWidth    = nullptr;
    QSpinBox    *m_spinHeight   = nullptr;
    QPushButton *m_btnClearRoi  = nullptr;

    // Format + Recording
    QComboBox   *m_comboFormat   = nullptr;
    QSpinBox    *m_spinFrames    = nullptr;
    QComboBox   *m_comboDuration = nullptr;
    QLineEdit   *m_txtFilename   = nullptr;
    QLabel      *m_lblSaveDir    = nullptr;
    QPushButton *m_btnSaveDir    = nullptr;

    // Histogram + display levels
    HistogramWidget *m_histogramWidget = nullptr;
    QCheckBox       *m_chkHistLog      = nullptr;
    QSlider         *m_sliderBlack     = nullptr;
    QSlider         *m_sliderWhite     = nullptr;
    QLabel          *m_lblBlack        = nullptr;
    QLabel          *m_lblWhite        = nullptr;

    // Sampling calculator (inputs from settings, results in toolbar)
    QDoubleSpinBox *m_spinDiameter      = nullptr;
    QDoubleSpinBox *m_spinFocalLength   = nullptr;
    QDoubleSpinBox *m_spinPixelSize     = nullptr;
    QDoubleSpinBox *m_spinWavelength    = nullptr;
    QLabel         *m_lblSampling       = nullptr;
    QLabel         *m_lblSamplingFactor = nullptr;

    // Status bar
    QLabel  *m_lblMem  = nullptr;
    QLabel  *m_lblFps  = nullptr;
    QLabel  *m_lblMbps = nullptr;
    double   m_fpsAvg  = -1.0;

    // Global toolbar — SSM quick controls
    QLabel         *m_lblSsmCurrent    = nullptr;
    QDoubleSpinBox *m_spinSsmThreshold = nullptr;
    QSpinBox       *m_spinSsmConsec    = nullptr;
    QCheckBox      *m_chkSsmTrigger    = nullptr;

    // SSM tab panel
    QComboBox      *m_comboSsmPort   = nullptr;
    QComboBox      *m_comboSsmBaud   = nullptr;
    QPushButton    *m_btnSsm         = nullptr;
    QLabel         *m_lblSsmInput    = nullptr;
    QLabel         *m_lblSsmMean     = nullptr;
    SeePlot        *m_ssmPlot        = nullptr;
    QComboBox      *m_comboSsmRange  = nullptr;
    QLabel         *m_lblSsmTrigger  = nullptr;
    QPushButton    *m_btnSsmLog      = nullptr;
    QLabel         *m_lblSsmLogPath  = nullptr;
    QCheckBox      *m_chkSsmRaw      = nullptr;
    QPlainTextEdit *m_txtSsmRaw      = nullptr;

    // ── Backend objects ───────────────────────────────────────────────────────
    AppSettings      m_settings;
    CameraInterface *m_camera        = nullptr;
    FrameGrabber    *m_grabber       = nullptr;
    SSMReader       *m_ssmReader     = nullptr;
    QTimer          *m_ssmPortTimer  = nullptr;
    QVector<double>  m_ssmHistory;
    int              m_ssmTriggerCount = 0;
    int              m_ssmConsecutive  = 0;

    // SSM CSV logging
    QFile       m_ssmLogFile;
    QTextStream m_ssmLogStream;

    // SER player (embedded as tab 3)
    SerPlayerDialog *m_serPlayer = nullptr;
};
