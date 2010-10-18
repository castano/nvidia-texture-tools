#include "compressdialog.h"
#include "ui_compressdialog.h"

#include <QtGui/QApplication>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    CompressDialog dialog("");

    return dialog.exec();
}



CompressDialog::CompressDialog(const QString & fileName, QWidget *parent) : QDialog(parent)
{
    ui.setupUi(this);

    //connect(ui.openButton, SIGNAL(clicked()), this, SLOT(openClicked()));
    connect(ui.generateMipmapsCheckBox, SIGNAL(stateChanged(int)), this, SLOT(generateMipmapsChanged(int)));
    connect(ui.mipmapFilterComboBox, SIGNAL(activated(QString)), this, SLOT(mipmapFilterChanged(QString)));
    //connect(ui.mipmapFilterSettings, SIGNAL(clicked()), this, SLOT(mipmapFilterSettingsShow()));

    connect(ui.formatComboBox, SIGNAL(activated(QString)), this, SLOT(formatChanged(QString)));


    connect(ui.redSpinBox, SIGNAL(valueChanged(double)), this, SLOT(colorWeightChanged()));
    connect(ui.greenSpinBox, SIGNAL(valueChanged(double)), this, SLOT(colorWeightChanged()));
    connect(ui.blueSpinBox, SIGNAL(valueChanged(double)), this, SLOT(colorWeightChanged()));
    connect(ui.uniformButton, SIGNAL(toggled(bool)), this, SLOT(uniformWeightToggled(bool)));
    connect(ui.luminanceButton, SIGNAL(toggled(bool)), this, SLOT(luminanceWeightToggled(bool)));

    //connect(ui.rgbMapRadioButton, SIGNAL(toggled(bool)), this, SLOT(colorModeChanged()));
    //connect(ui.normalMapRadioButton, SIGNAL(toggled(bool)), this, SLOT(normalMapModeChanged(bool)));
}

CompressDialog::~CompressDialog()
{
}


void CompressDialog::openClicked()
{
    // @@ What is openButton?
}


void CompressDialog::generateMipmapsChanged(int state)
{
    Q_UNUSED(state);

    bool generateMipmapEnabled = ui.generateMipmapsCheckBox->isChecked();

    ui.mipmapFilterLabel->setEnabled(generateMipmapEnabled);
    ui.mipmapFilterComboBox->setEnabled(generateMipmapEnabled);
    ui.limitMipmapsCheckBox->setEnabled(generateMipmapEnabled);

    bool enableFilterSettings = (ui.mipmapFilterComboBox->currentText() == "Kaiser");
    ui.mipmapFilterSettings->setEnabled(generateMipmapEnabled && enableFilterSettings);

    bool enableMaxLevel = ui.limitMipmapsCheckBox->isChecked();
    ui.maxLevelLabel->setEnabled(generateMipmapEnabled && enableMaxLevel);
    ui.maxLevelSpinBox->setEnabled(generateMipmapEnabled && enableMaxLevel);
}

void CompressDialog::mipmapFilterChanged(QString name)
{
    bool enableFilterSettings = (name == "Kaiser");
    ui.mipmapFilterSettings->setEnabled(enableFilterSettings);
}

void CompressDialog::formatChanged(QString format)
{
    if (format == "Uncompressed") {
        ui.formatOptions->setCurrentIndex(1);
    }
    else {
        ui.formatOptions->setCurrentIndex(0);
    }
}

void CompressDialog::colorWeightChanged()
{
    double r = ui.redSpinBox->value();
    double g = ui.greenSpinBox->value();
    double b = ui.blueSpinBox->value();

    bool uniform = (r == 1.0 && g == 1.0 && b == 1.0);
    bool luminance = (r == 0.3 && g == 0.59 && b == 0.11);

    ui.uniformButton->setChecked(uniform);
    ui.luminanceButton->setChecked(luminance);
}

void CompressDialog::uniformWeightToggled(bool checked)
{
    if (checked)
    {
        ui.redSpinBox->setValue(1.0);
        ui.greenSpinBox->setValue(1.0);
        ui.blueSpinBox->setValue(1.0);
    }
}

void CompressDialog::luminanceWeightToggled(bool checked)
{
    if (checked)
    {
        ui.redSpinBox->setValue(0.3);
        ui.greenSpinBox->setValue(0.59);
        ui.blueSpinBox->setValue(0.11);
    }
}

void CompressDialog::normalMapModeChanged(bool checked)
{
    //ui.alphaModeGroupBox->setEnabled(!checked);
    //ui.inputGammaSpinBox->setEnabled(!checked);
    //ui.inputGammaLabel->setEnabled(!checked);
    //ui.outputGammaSpinBox->setEnabled(!checked);
    //ui.outputGammaLabel->setEnabled(!checked);
}
