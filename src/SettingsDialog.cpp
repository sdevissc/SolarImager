#include "SettingsDialog.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QLabel>
#include <QDoubleSpinBox>
#include <QSpinBox>
#include <QComboBox>
#include <QLineEdit>
#include <QListWidget>
#include <QPushButton>
#include <QDialogButtonBox>
#include <QFileDialog>
#include <QInputDialog>
#include <QMessageBox>
#include <QTabWidget>
#include <QDir>
#include <QStandardPaths>

SettingsDialog::SettingsDialog(const AppSettings &current, QWidget *parent)
    : QDialog(parent), m_settings(current)
{
    setWindowTitle("Settings");
    setMinimumWidth(520);
    buildUi();
    populateFromSettings(current);
    refreshProfileList();
}

AppSettings SettingsDialog::settings() const
{
    AppSettings s;

    // Telescope
    s.telDiameter      = m_spinDiameter->value();
    s.telFocalLength   = m_spinFocalLength->value();

    // Camera
    s.camPixelSize     = m_spinPixelSize->value();
    s.camWavelength    = m_spinWavelength->value();
    s.camDefaultFormat = m_comboFormat->currentText();
    s.camDefaultBitDepth = m_comboBitDepth->currentText().toInt();

    // Display
    s.displayPalette   = m_comboPalette->currentText();
    s.displayZoom      = m_comboZoom->currentText();

    // Recording
    s.recDirectory     = m_txtRecDir->text();
    s.recDuration      = m_spinDuration->value();
    s.recDurationMode  = m_comboDurMode->currentText();

    // SSM
    s.ssmThreshold     = m_spinThreshold->value();
    s.ssmConsec        = m_spinConsec->value();
    s.ssmBaud          = m_comboBaud->currentText();

    s.profileName      = m_settings.profileName;
    return s;
}

void SettingsDialog::buildUi()
{
    auto *root = new QVBoxLayout(this);

    // ── Tabs ─────────────────────────────────────────────────────────────────
    auto *tabs = new QTabWidget();

    // ── Tab 1: Telescope & Camera ─────────────────────────────────────────────
    auto *tab1 = new QWidget();
    auto *t1   = new QVBoxLayout(tab1);

    auto *telGrp = new QGroupBox("Telescope");
    auto *telLay = new QGridLayout(telGrp);
    telLay->addWidget(new QLabel("Diameter (mm):"),      0, 0);
    m_spinDiameter = new QDoubleSpinBox();
    m_spinDiameter->setRange(10, 2000); m_spinDiameter->setDecimals(0);
    telLay->addWidget(m_spinDiameter,                    0, 1);
    telLay->addWidget(new QLabel("Focal length (mm):"),  1, 0);
    m_spinFocalLength = new QDoubleSpinBox();
    m_spinFocalLength->setRange(100, 20000); m_spinFocalLength->setDecimals(0);
    telLay->addWidget(m_spinFocalLength,                 1, 1);
    t1->addWidget(telGrp);

    auto *camGrp = new QGroupBox("Camera");
    auto *camLay = new QGridLayout(camGrp);
    camLay->addWidget(new QLabel("Pixel size (µm):"),    0, 0);
    m_spinPixelSize = new QDoubleSpinBox();
    m_spinPixelSize->setRange(1.0, 30.0);
    m_spinPixelSize->setDecimals(2); m_spinPixelSize->setSingleStep(0.1);
    camLay->addWidget(m_spinPixelSize,                   0, 1);
    camLay->addWidget(new QLabel(u8"Wavelength \u03BB (nm):"), 1, 0);
    m_spinWavelength = new QDoubleSpinBox();
    m_spinWavelength->setRange(300, 1100); m_spinWavelength->setDecimals(0);
    camLay->addWidget(m_spinWavelength,                  1, 1);
    camLay->addWidget(new QLabel("Default format:"),     2, 0);
    m_comboFormat = new QComboBox();
    m_comboFormat->addItems({"RAW8", "RAW16", "Mono8", "Mono12"});
    camLay->addWidget(m_comboFormat,                     2, 1);
    camLay->addWidget(new QLabel("Default bit depth:"),  3, 0);
    m_comboBitDepth = new QComboBox();
    m_comboBitDepth->addItems({"8", "16"});
    camLay->addWidget(m_comboBitDepth,                   3, 1);
    t1->addWidget(camGrp);
    t1->addStretch();
    tabs->addTab(tab1, "Telescope & Camera");

    // ── Tab 2: Display & Recording ────────────────────────────────────────────
    auto *tab2 = new QWidget();
    auto *t2   = new QVBoxLayout(tab2);

    auto *dispGrp = new QGroupBox("Display");
    auto *dispLay = new QGridLayout(dispGrp);
    dispLay->addWidget(new QLabel("Default palette:"),   0, 0);
    m_comboPalette = new QComboBox();
    m_comboPalette->addItems({"Original","Inverted","Rainbow","Red Hot","Cool"});
    dispLay->addWidget(m_comboPalette,                   0, 1);
    dispLay->addWidget(new QLabel("Default zoom:"),      1, 0);
    m_comboZoom = new QComboBox();
    m_comboZoom->addItems({"Fit","25%","33%","50%","75%","100%","150%","200%"});
    dispLay->addWidget(m_comboZoom,                      1, 1);
    t2->addWidget(dispGrp);

    auto *recGrp = new QGroupBox("Recording");
    auto *recLay = new QGridLayout(recGrp);
    recLay->addWidget(new QLabel("Save directory:"),     0, 0);
    auto *dirRow = new QHBoxLayout();
    m_txtRecDir = new QLineEdit();
    m_txtRecDir->setPlaceholderText(QDir::homePath());
    dirRow->addWidget(m_txtRecDir);
    auto *btnBrowse = new QPushButton("Browse…");
    btnBrowse->setFixedWidth(80);
    connect(btnBrowse, &QPushButton::clicked, this, [this] {
        QString d = QFileDialog::getExistingDirectory(
            this, "Select save directory", m_txtRecDir->text());
        if (!d.isEmpty()) m_txtRecDir->setText(d);
    });
    dirRow->addWidget(btnBrowse);
    recLay->addLayout(dirRow,                            0, 1);
    recLay->addWidget(new QLabel("Default duration:"),   1, 0);
    auto *durRow = new QHBoxLayout();
    m_spinDuration = new QSpinBox();
    m_spinDuration->setRange(1, 100000); m_spinDuration->setValue(100);
    durRow->addWidget(m_spinDuration);
    m_comboDurMode = new QComboBox();
    m_comboDurMode->addItems({"Frames","Seconds"});
    m_comboDurMode->setFixedWidth(80);
    durRow->addWidget(m_comboDurMode);
    recLay->addLayout(durRow,                            1, 1);
    t2->addWidget(recGrp);
    t2->addStretch();
    tabs->addTab(tab2, "Display & Recording");

    // ── Tab 3: SSM ────────────────────────────────────────────────────────────
    auto *tab3 = new QWidget();
    auto *t3   = new QVBoxLayout(tab3);

    auto *ssmGrp = new QGroupBox("SSM Defaults");
    auto *ssmLay = new QGridLayout(ssmGrp);
    ssmLay->addWidget(new QLabel("Threshold (arcsec):"), 0, 0);
    m_spinThreshold = new QDoubleSpinBox();
    m_spinThreshold->setRange(0.1, 10.0);
    m_spinThreshold->setDecimals(2); m_spinThreshold->setSingleStep(0.1);
    ssmLay->addWidget(m_spinThreshold,                   0, 1);
    ssmLay->addWidget(new QLabel("Consecutive samples:"), 1, 0);
    m_spinConsec = new QSpinBox();
    m_spinConsec->setRange(1, 100);
    ssmLay->addWidget(m_spinConsec,                      1, 1);
    ssmLay->addWidget(new QLabel("Default baud rate:"),  2, 0);
    m_comboBaud = new QComboBox();
    m_comboBaud->addItems({"9600","19200","38400","57600","115200"});
    ssmLay->addWidget(m_comboBaud,                       2, 1);
    t3->addWidget(ssmGrp);
    t3->addStretch();
    tabs->addTab(tab3, "SSM");

    root->addWidget(tabs);

    // ── Profile management ────────────────────────────────────────────────────
    auto *profGrp = new QGroupBox("Profiles");
    auto *profLay = new QHBoxLayout(profGrp);

    m_profileList = new QListWidget();
    m_profileList->setFixedHeight(90);
    profLay->addWidget(m_profileList, 1);

    auto *profBtns = new QVBoxLayout();
    m_txtProfileName = new QLineEdit();
    m_txtProfileName->setPlaceholderText("Profile name…");
    m_txtProfileName->setFixedWidth(140);
    profBtns->addWidget(m_txtProfileName);

    m_btnSave = new QPushButton("💾  Save");
    m_btnLoad = new QPushButton("📂  Load");
    m_btnDelete = new QPushButton("🗑  Delete");
    m_btnSave->setFixedWidth(140);
    m_btnLoad->setFixedWidth(140);
    m_btnDelete->setFixedWidth(140);
    profBtns->addWidget(m_btnSave);
    profBtns->addWidget(m_btnLoad);
    profBtns->addWidget(m_btnDelete);
    profBtns->addStretch();
    profLay->addLayout(profBtns);

    connect(m_btnSave,   &QPushButton::clicked, this, &SettingsDialog::onSaveProfile);
    connect(m_btnLoad,   &QPushButton::clicked, this, &SettingsDialog::onLoadProfile);
    connect(m_btnDelete, &QPushButton::clicked, this, &SettingsDialog::onDeleteProfile);
    connect(m_profileList, &QListWidget::currentTextChanged,
            this, &SettingsDialog::onProfileSelected);

    root->addWidget(profGrp);

    // ── OK / Cancel ───────────────────────────────────────────────────────────
    auto *btns = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    connect(btns, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(btns, &QDialogButtonBox::rejected, this, &QDialog::reject);
    root->addWidget(btns);
}

void SettingsDialog::populateFromSettings(const AppSettings &s)
{
    m_spinDiameter->setValue(s.telDiameter);
    m_spinFocalLength->setValue(s.telFocalLength);
    m_spinPixelSize->setValue(s.camPixelSize);
    m_spinWavelength->setValue(s.camWavelength);
    m_comboFormat->setCurrentText(s.camDefaultFormat);
    m_comboBitDepth->setCurrentText(QString::number(s.camDefaultBitDepth));
    m_comboPalette->setCurrentText(s.displayPalette);
    m_comboZoom->setCurrentText(s.displayZoom);
    m_txtRecDir->setText(s.recDirectory);
    m_spinDuration->setValue(s.recDuration);
    m_comboDurMode->setCurrentText(s.recDurationMode);
    m_spinThreshold->setValue(s.ssmThreshold);
    m_spinConsec->setValue(s.ssmConsec);
    m_comboBaud->setCurrentText(s.ssmBaud);
    m_txtProfileName->setText(s.profileName);
}

void SettingsDialog::refreshProfileList()
{
    m_profileList->clear();
    for (const QString &n : AppSettings::availableProfiles())
        m_profileList->addItem(n);
}

void SettingsDialog::onProfileSelected()
{
    auto *item = m_profileList->currentItem();
    if (item) m_txtProfileName->setText(item->text());
}

void SettingsDialog::onSaveProfile()
{
    QString name = m_txtProfileName->text().trimmed();
    if (name.isEmpty()) {
        QMessageBox::warning(this, "Save Profile", "Please enter a profile name.");
        return;
    }
    AppSettings s = settings();
    s.profileName = name;
    if (s.save(name)) {
        m_settings.profileName = name;
        refreshProfileList();
        // Select saved profile in list
        auto items = m_profileList->findItems(name, Qt::MatchExactly);
        if (!items.isEmpty()) m_profileList->setCurrentItem(items.first());
    }
}

void SettingsDialog::onLoadProfile()
{
    auto *item = m_profileList->currentItem();
    if (!item) {
        QMessageBox::information(this, "Load Profile",
                                 "Select a profile from the list first.");
        return;
    }
    AppSettings s = AppSettings::load(item->text());
    m_settings = s;
    populateFromSettings(s);
}

void SettingsDialog::onDeleteProfile()
{
    auto *item = m_profileList->currentItem();
    if (!item) return;
    const QString name = item->text();
    if (QMessageBox::question(this, "Delete Profile",
            QString("Delete profile \"%1\"?").arg(name))
        != QMessageBox::Yes) return;

    QFile::remove(AppSettings::profileDir() + "/" + name + ".json");
    refreshProfileList();
}
