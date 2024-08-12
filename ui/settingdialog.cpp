/*
* ========================================================================== *
*                                                                            *
*    This file is part of the Openterface Mini KVM App QT version            *
*                                                                            *
*    Copyright (C) 2024   <info@openterface.com>                             *
*                                                                            *
*    This program is free software: you can redistribute it and/or modify    *
*    it under the terms of the GNU General Public License as published by    *
*    the Free Software Foundation version 3.                                 *
*                                                                            *
*    This program is distributed in the hope that it will be useful, but     *
*    WITHOUT ANY WARRANTY; without even the implied warranty of              *
*    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU        *
*    General Public License for more details.                                *
*                                                                            *
*    You should have received a copy of the GNU General Public License       *
*    along with this program. If not, see <http://www.gnu.org/licenses/>.    *
*                                                                            *
* ========================================================================== *
*/

#include "settingdialog.h"
#include "ui_settingdialog.h"
#include "ui/fpsspinbox.h"
#include "global.h"


#include <QCamera>
#include <QCameraDevice>
#include <QCameraFormat>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QDir>
#include <QFileDialog>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QCheckBox>
#include <QRegularExpression>
#include <QToolButton>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QStackedWidget>
#include <QDebug>
#include <QLoggingCategory>


SettingDialog::SettingDialog(QCamera *_camera, QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::SettingDialog)
    , settingTree(new QTreeWidget(this))
    , stackedWidget(new QStackedWidget(this))
    , buttonWidget(new QWidget(this))
    , logPage(nullptr)
    , videoPage(nullptr)
    , audioPage(nullptr)
    , camera(_camera)

{
    ui->setupUi(this);
    createSettingTree();
    createPages();
    createButtons();
    createLayout();
    setWindowTitle(tr("Preferences"));

    // Connect the tree widget's currentItemChanged signal to a slot
    connect(settingTree, &QTreeWidget::currentItemChanged, this, &SettingDialog::changePage);
}

SettingDialog::~SettingDialog()
{
    delete ui;
    // Ensure all dynamically allocated memory is freed
    qDeleteAll(settingTree->invisibleRootItem()->takeChildren());
}

void SettingDialog::createSettingTree() {
    // qDebug() << "creating setting Tree";
    settingTree->setColumnCount(1);
    // settingTree->setHeaderLabels(QStringList(tr("general")));
    settingTree->setHeaderHidden(true);
    settingTree->setSelectionMode(QAbstractItemView::SingleSelection);
    
    settingTree->setMaximumSize(QSize(120, 1000));
    settingTree->setRootIsDecorated(false);
    
    // QStringList names = {"Log"};
    QStringList names = {"Log", "Video", "Audio"};
    for (const QString &name : names) {     // add item to setting tree
        QTreeWidgetItem *item = new QTreeWidgetItem(settingTree);
        item->setText(0, name);
    }
}

void SettingDialog::createLogPage() {
    logPage = new QWidget();

    // Create checkbox for log
    QCheckBox *coreCheckBox = new QCheckBox("Core");
    QCheckBox *serialCheckBox = new QCheckBox("Serial");
    QCheckBox *uiCheckBox = new QCheckBox("Ui");
    QCheckBox *hostCheckBox = new QCheckBox("Host");
    coreCheckBox->setObjectName("core");
    serialCheckBox->setObjectName("serial");
    uiCheckBox->setObjectName("ui");
    hostCheckBox->setObjectName("host");

    QHBoxLayout *logCheckboxLayout = new QHBoxLayout();
    logCheckboxLayout->addWidget(coreCheckBox);
    logCheckboxLayout->addWidget(serialCheckBox);
    logCheckboxLayout->addWidget(uiCheckBox);
    logCheckboxLayout->addWidget(hostCheckBox);

    QLabel *logLabel = new QLabel("General log setting");

    QVBoxLayout *logLayout = new QVBoxLayout(logPage);
    logLayout->addWidget(logLabel);
    logLayout->addLayout(logCheckboxLayout);
    logLayout->addStretch();
}

void SettingDialog::createVideoPage() {
    videoPage = new QWidget();

    QLabel *videoLabel = new QLabel("General video setting");
    QLabel *resolutionsLabel = new QLabel("Capture resolutions: ");
    QComboBox *videoFormatBox = new QComboBox();
    videoFormatBox->setObjectName("videoFormatBox");

    QLabel *framerateLabel = new QLabel("Framerate: ");
    FpsSpinBox *fpsSpinBox = new FpsSpinBox();
    fpsSpinBox->setObjectName("fpsSpinBox");
    QSlider *fpsSlider = new QSlider();
    fpsSlider->setObjectName("fpsSlider");
    fpsSlider->setOrientation(Qt::Horizontal);

    QHBoxLayout *hBoxLayout = new QHBoxLayout();
    hBoxLayout->addWidget(fpsSpinBox);
    hBoxLayout->addWidget(fpsSlider);

    QLabel *formatLabel = new QLabel("Pixel format: ");
    QComboBox *pixelFormatBox = new QComboBox();
    pixelFormatBox->setObjectName("pixelFormatBox");

    QVBoxLayout *videoLayout = new QVBoxLayout(videoPage);
    videoLayout->addWidget(videoLabel);
    videoLayout->addWidget(resolutionsLabel);
    videoLayout->addWidget(videoFormatBox);
    videoLayout->addWidget(framerateLabel);
    videoLayout->addLayout(hBoxLayout);
    videoLayout->addWidget(formatLabel);
    videoLayout->addWidget(pixelFormatBox);
    videoLayout->addStretch();
   
    if (camera  != nullptr && !camera->cameraDevice().isNull() ){
        const QList<QCameraFormat> videoFormats = camera->cameraDevice().videoFormats();
        // for (const QCameraFormat &format : videoFormats) {
        //     qDebug() << "Resolution: " << format.resolution();
        //     qDebug() << "Frame rate range: " << format.minFrameRate() << " - " << format.maxFrameRate();
        //     qDebug() << "Pixel format: " << format.pixelFormat();
        //     qDebug() << "--------------------------------------------------";
        // }
        populateResolutionBox(videoFormats);
        connect(videoFormatBox, &QComboBox::currentIndexChanged, [this, videoFormatBox](int /*index*/){
            this->setFpsRange(boxValue(videoFormatBox).value<std::set<int>>());

            QString resolutionText = videoFormatBox->currentText();
            QStringList resolutionParts = resolutionText.split(' ').first().split('x');
            m_currentResolution = QSize(resolutionParts[0].toInt(), resolutionParts[1].toInt());
        });
        connect(fpsSlider, &QSlider::valueChanged, fpsSpinBox, &QSpinBox::setValue);
        connect(fpsSpinBox, &QSpinBox::valueChanged, fpsSlider, &QSlider::setValue);
        const std::set<int> fpsValues = boxValue(videoFormatBox).value<std::set<int>>();

        setFpsRange(fpsValues);
        QString resolutionText = videoFormatBox->currentText();
        QStringList resolutionParts = resolutionText.split(' ').first().split('x');
        m_currentResolution = QSize(resolutionParts[0].toInt(), resolutionParts[1].toInt());

    }else {
        qWarning() << "Camera or CameraDevice is not valid.";
    }
}

QVariant SettingDialog::boxValue(const QComboBox *box) const
{
    const int idx = box->currentIndex();
    return idx != -1 ? box->itemData(idx) : QVariant{};
}

void SettingDialog::applyVideoSettings(){

}

QCameraFormat SettingDialog::getVideoFormat(const QSize &resolution, int frameRate, QVideoFrameFormat::PixelFormat pixelFormat) const{
    VideoFormatKey key = {resolution, frameRate, pixelFormat};
    auto it = videoFormatMap.find(key);
    if (it != videoFormatMap.end()) {
        return it->second;
    }
    // Handle the case where the format is not found
    return QCameraFormat();
}

void SettingDialog::setFpsRange(const std::set<int> &fpsValues){
    if (!fpsValues.empty()) {
        int minFps = *fpsValues.begin(); // First element is the minimum
        int maxFps = *fpsValues.rbegin(); // Last element is the maximum

        // Set the range for the slider and spin box
        QSlider *fpsSlider = videoPage->findChild<QSlider*>("fpsSlider");
        FpsSpinBox *fpsSpinBox = videoPage->findChild<FpsSpinBox*>("fpsSpinBox");
        fpsSlider->setRange(minFps, maxFps);
        fpsSlider->setValue(maxFps);
        fpsSpinBox->setRange(minFps, maxFps);
        fpsSpinBox->setValidValues(fpsValues);

         // Adjust the current value of the slider if it's out of the new range
        int currentSliderValue = fpsSlider->value();
        qDebug() << "Set fps current value" << currentSliderValue;
        if (fpsValues.find(currentSliderValue) == fpsValues.end()) {
            // If current value is not in set, set to the maximum value
            int maxFps = *fpsValues.rbegin(); // Get the maximum value from the set
            fpsSlider->setValue(maxFps);
        }
        connect(fpsSlider, &QSlider::valueChanged, this, &SettingDialog::onFpsSliderValueChanged);
    }
}

void SettingDialog::onFpsSliderValueChanged(int value){
    // This ensures that even if the slider is manually moved, it snaps to valid fpsValues
    QComboBox *videoFormatBox  = videoPage->findChild<QComboBox*>("videoFormatBox");
    QSlider *fpsSlider = videoPage->findChild<QSlider*>("fpsSpinBox");
    // const std::set<int> fpsValues = boxValue(videoFormatBox).value<std::set<int>>();
    const std::set<int> fpsValues = boxValue(videoFormatBox).value<std::set<int>>();
    if (fpsValues.find(value) == fpsValues.end()) {
        auto lower = fpsValues.lower_bound(value);
        auto upper = fpsValues.upper_bound(value);
        int nearestValue = value;

        if (lower != fpsValues.end() && upper != fpsValues.begin()) {
            upper--; // Move one step back to get the closest lesser value
            nearestValue = (value - *upper <= *lower - value) ? *upper : *lower;
        } else if (lower != fpsValues.end()) {
            nearestValue = *lower;
        } else if (upper != fpsValues.begin()) {
            nearestValue = *(--upper);
        }
        
        fpsSlider->setValue(nearestValue);
    }
}

void SettingDialog::populateResolutionBox(const QList<QCameraFormat> &videoFormats){
    std::map<QSize, std::set<int>, QSizeComparator> resolutionSampleRates;

    // Process videoFormats to fill resolutionSampleRates and videoFormatMap
    for (const QCameraFormat &format : videoFormats) {
        QSize resolution = format.resolution();
        int frameRate = format.minFrameRate();
        QVideoFrameFormat::PixelFormat pixelFormat = format.pixelFormat();

        VideoFormatKey key = {resolution, frameRate, pixelFormat};
        videoFormatMap[key] = format;

        resolutionSampleRates[resolution].insert(frameRate);
    }

    // Populate videoFormatBox with consolidated information
    for (const auto &entry : resolutionSampleRates) {
        const QSize &resolution = entry.first;
        const std::set<int> &sampleRates = entry.second;

        // Convert sampleRates to QStringList for printing
        QStringList sampleRatesList;
        for (int rate : sampleRates) {
            sampleRatesList << QString::number(rate);
        }

        // Print all sampleRates
        qDebug() << "Resolution:" << resolution << "Sample Rates:" << sampleRatesList.join(", ");

        if (!sampleRates.empty()) {
            int minSampleRate = *std::begin(sampleRates); // First element is the smallest
            int maxSampleRate = *std::rbegin(sampleRates); // Last element is the largest
            QString itemText = QString("%1x%2 [%3 - %4 Hz]").arg(resolution.width()).arg(resolution.height()).arg(minSampleRate).arg(maxSampleRate);

            // Convert the entire set to QVariant
            QVariant sampleRatesVariant = QVariant::fromValue<std::set<int>>(sampleRates);

            
            QComboBox *videoFormatBox = videoPage->findChild<QComboBox*>("videoFormatBox");
            videoFormatBox->addItem(itemText, sampleRatesVariant);
        }
    }
}


void SettingDialog::createAudioPage() {
    audioPage = new QWidget();

    QLabel *audioLabel = new QLabel("General audio setting");
    QLabel *audioCodecLabel = new QLabel("Audio Codec: ");
    QComboBox *audioCodecBox = new QComboBox();
    audioCodecBox->setObjectName("audioCodecBox");

    QLabel *audioSampleRateLabel = new QLabel("Sample Rate: ");
    QSpinBox *audioSampleRateBox = new QSpinBox();
    audioSampleRateBox->setObjectName("audioSampleRateBox");
    audioSampleRateBox->setEnabled(false);

    QLabel *qualityLabel = new QLabel("Quality: ");
    QSlider *qualitySlider = new QSlider();
    qualitySlider->setObjectName("qualitySlider");
    qualitySlider->setOrientation(Qt::Horizontal);

    QLabel *fileFormatLabel = new QLabel("File Format: ");
    QComboBox *containerFormatBox = new QComboBox();
    containerFormatBox->setObjectName("containerFormatBox");

    QVBoxLayout *audioLayout = new QVBoxLayout(audioPage);
    audioLayout->addWidget(audioLabel);
    audioLayout->addWidget(audioCodecLabel);
    audioLayout->addWidget(audioCodecBox);
    audioLayout->addWidget(audioSampleRateLabel);
    audioLayout->addWidget(audioSampleRateBox);
    audioLayout->addWidget(qualityLabel);
    audioLayout->addWidget(qualitySlider);
    audioLayout->addWidget(fileFormatLabel);
    audioLayout->addWidget(containerFormatBox);
    audioLayout->addStretch();
}

void SettingDialog::createPages() {
    createLogPage();
    createVideoPage();
    createAudioPage();


    // Add pages to the stacked widget
    stackedWidget->addWidget(logPage);
    stackedWidget->addWidget(videoPage);
    stackedWidget->addWidget(audioPage);
}


void SettingDialog::createButtons(){
    QPushButton *okButton = new QPushButton("OK");
    QPushButton *applyButton = new QPushButton("Apply");
    QPushButton *cancelButton = new QPushButton("Cancel");

    okButton->setFixedSize(80, 30);
    applyButton->setFixedSize(80, 30);
    cancelButton->setFixedSize(80, 30);

    QHBoxLayout *buttonLayout = new QHBoxLayout(buttonWidget);
    buttonLayout->addStretch();
    buttonLayout->addWidget(okButton);
    buttonLayout->addWidget(applyButton);
    buttonLayout->addWidget(cancelButton);

    connect(okButton, &QPushButton::clicked, this, &SettingDialog::handleOkButton);
    connect(cancelButton, &QPushButton::clicked, this, &QDialog::reject);
    connect(applyButton, &QPushButton::clicked, this, &SettingDialog::applyAccrodingPage);
}

void SettingDialog::createLayout() {

    QHBoxLayout *selectLayout = new QHBoxLayout;
    selectLayout->addWidget(settingTree);
    selectLayout->addWidget(stackedWidget);

    QVBoxLayout *mainLayout = new QVBoxLayout;
    mainLayout->addLayout(selectLayout);
    mainLayout->addWidget(buttonWidget);
    
    setLayout(mainLayout);
}

void SettingDialog::changePage(QTreeWidgetItem *current, QTreeWidgetItem *previous) {
    if (!current)
        current = previous;

    // Switch page based on the selected item
    QString itemText = current->text(0);
    qDebug() << "Selected item:" << itemText;

    if (itemText == "Log") {
        QMetaObject::invokeMethod(this, [this]() {
            stackedWidget->setCurrentIndex(0);
        }, Qt::QueuedConnection);
    } else if (itemText == "Video") {
        QMetaObject::invokeMethod(this, [this]() {
            stackedWidget->setCurrentIndex(1);
        }, Qt::QueuedConnection);
    } else if (itemText == "Audio") {
        QMetaObject::invokeMethod(this, [this]() {
            stackedWidget->setCurrentIndex(2);
        }, Qt::QueuedConnection);
    }
}

void SettingDialog::setLogCheckBox(){
    QCheckBox *coreCheckBox = findChild<QCheckBox*>("core");
    QCheckBox *serialCheckBox = findChild<QCheckBox*>("serial");
    QCheckBox *uiCheckBox = findChild<QCheckBox*>("ui");
    QCheckBox *hostCheckBox = findChild<QCheckBox*>("host");
    
    coreCheckBox->setChecked(true);
    serialCheckBox->setChecked(true);
    uiCheckBox->setChecked(true);
    hostCheckBox->setChecked(true);
}

void SettingDialog::readCheckBoxState() {
    QCheckBox *coreCheckBox = findChild<QCheckBox*>("core");
    QCheckBox *serialCheckBox = findChild<QCheckBox*>("serial");
    QCheckBox *uiCheckBox = findChild<QCheckBox*>("ui");
    QCheckBox *hostCheckBox = findChild<QCheckBox*>("host");

    // set the log filter value by check box
    QString logFilter = "";

    if (coreCheckBox && coreCheckBox->isChecked()) {
        logFilter += "opf.core.*=true\n";
    } else {
        logFilter += "opf.core.*=false\n";
    }

    if (uiCheckBox && uiCheckBox->isChecked()) {
        logFilter += "opf.ui.*=true\n";
    } else {
        logFilter += "opf.ui.*=false\n";
    }

    if (hostCheckBox && hostCheckBox->isChecked()) {
        logFilter += "opf.host.*=true\n";
    } else {
        logFilter += "opf.host.*=false\n";
    }

    if (serialCheckBox && serialCheckBox->isChecked()) {
        logFilter += "opf.core.serial=true\n";
    } else {
        logFilter += "opf.core.serial=false\n";
    }

    QLoggingCategory::setFilterRules(logFilter);
}

void SettingDialog::applyAccrodingPage(){
    int currentPageIndex = stackedWidget->currentIndex();
    switch (currentPageIndex)
    {
        // sequence Log Video Audio
        case 0:
            readCheckBoxState();
            break;
        case 1:
            
            break;
        case 2:

            break;
        default:
            break;
    }
}

void SettingDialog::handleOkButton() {
    readCheckBoxState();
    accept();
}
