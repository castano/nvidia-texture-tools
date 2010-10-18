#ifndef COMPRESSDIALOG_H
#define COMPRESSDIALOG_H

#include <QtGui/QDialog>

#include "ui_compressdialog.h"


class CompressDialog : public QDialog
{
    Q_OBJECT
public:
    explicit CompressDialog(const QString & fileName, QWidget *parent = 0);
    ~CompressDialog();

protected slots:

    void openClicked();
    void generateMipmapsChanged(int state);
    void mipmapFilterChanged(QString name);
    void formatChanged(QString format);

    void colorWeightChanged();
    void uniformWeightToggled(bool checked);
    void luminanceWeightToggled(bool checked);

    void normalMapModeChanged(bool checked);


private:
    Ui::CompressDialog ui;
};

#endif // COMPRESSDIALOG_H
