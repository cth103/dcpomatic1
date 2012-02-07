/*
     OpenDCP: Builds Digital Cinema Packages
     Copyright (c) 2010-2011 Terrence Meiczinger, All Rights Reserved

     This program is free software: you can redistribute it and/or modify
     it under the terms of the GNU General Public License as published by
     the Free Software Foundation, either version 3 of the License, or
     (at your option) any later version.

     This program is distributed in the hope that it will be useful,
     but WITHOUT ANY WARRANTY; without even the implied warranty of
     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
     GNU General Public License for more details.

     You should have received a copy of the GNU General Public License
     along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QtGui>
#include <QDir>
#include <QPixmap>
#include <stdio.h>
#include <opendcp.h>

void MainWindow::j2kConnectSlots()
{
    // connect slots
    connect(ui->stereoscopicCheckBox, SIGNAL(stateChanged(int)), this, SLOT(j2kSetStereoscopicState()));
    connect(ui->bwSlider,SIGNAL(valueChanged(int)),this, SLOT(j2kBwSliderUpdate()));
    connect(ui->encodeButton,SIGNAL(clicked()),this,SLOT(j2kStart()));
    connect(ui->profileComboBox,SIGNAL(currentIndexChanged(int)),this,SLOT(j2kCinemaProfileUpdate()));

    // set signal mapper to handle file dialogs
    signalMapper.setMapping(ui->inImageLeftButton, ui->inImageLeftEdit);
    signalMapper.setMapping(ui->inImageRightButton, ui->inImageRightEdit);
    signalMapper.setMapping(ui->outJ2kLeftButton, ui->outJ2kLeftEdit);
    signalMapper.setMapping(ui->outJ2kRightButton, ui->outJ2kRightEdit);

    // connect j2k signals
    connect(ui->inImageLeftButton, SIGNAL(clicked()),&signalMapper, SLOT(map()));
    connect(ui->inImageRightButton, SIGNAL(clicked()), &signalMapper, SLOT(map()));
    connect(ui->outJ2kLeftButton, SIGNAL(clicked()), &signalMapper, SLOT(map()));
    connect(ui->outJ2kRightButton, SIGNAL(clicked()), &signalMapper, SLOT(map()));

    // update file
    connect(ui->inImageLeftEdit, SIGNAL(textChanged(QString)),this,SLOT(j2kCheckLeftInputFiles()));
    connect(ui->inImageRightEdit, SIGNAL(textChanged(QString)),this,SLOT(j2kCheckRightInputFiles()));
    connect(ui->endSpinBox, SIGNAL(valueChanged(int)),this,SLOT(j2kUpdateEndSpinBox()));
}

void MainWindow::j2kSetStereoscopicState() {
    int value = ui->stereoscopicCheckBox->checkState();

    if (value) {
        ui->inImageLeft->setText(tr("Left:"));
        ui->outJ2kLeft->setText(tr("Left:"));
        ui->inImageRight->show();
        ui->inImageRightEdit->show();
        ui->inImageRightButton->show();
        ui->outJ2kRight->show();
        ui->outJ2kRightEdit->show();
        ui->outJ2kRightButton->show();

        ui->bwSlider->setValue(250);
    } else {
        ui->inImageLeft->setText(tr("Directory:"));
        ui->outJ2kLeft->setText(tr("Directory:"));
        ui->inImageRight->hide();
        ui->inImageRightEdit->hide();
        ui->inImageRightButton->hide();
        ui->outJ2kRight->hide();
        ui->outJ2kRightEdit->hide();
        ui->outJ2kRightButton->hide();
        ui->bwSlider->setValue(125);
    }
}

void MainWindow::j2kCinemaProfileUpdate() {
    if (ui->profileComboBox->currentIndex() == 0) {
#ifdef Q_WS_WIN
        //ui->threadsSpinBox->setMaximum(6);
#endif
        ui->threadsSpinBox->setMaximum(QThreadPool::globalInstance()->maxThreadCount());
        ui->threadsSpinBox->setValue(QThread::idealThreadCount());
    } else {
#ifdef Q_WS_WIN
        //ui->threadsSpinBox->setMaximum(2);
#endif
        ui->threadsSpinBox->setMaximum(QThreadPool::globalInstance()->maxThreadCount());
        ui->threadsSpinBox->setValue(QThread::idealThreadCount());
    }
}

void MainWindow::j2kBwSliderUpdate() {
    int bw = ui->bwSlider->value();
    QString string;

    string.sprintf("%d mb/s",bw);
    ui->bwValueLabel->setText(string);
}

// globals for threads
opendcp_t *context;
int iterations = 0;
QFileInfoList inLeftList;
QFileInfoList inRightList;
QString outLeftDir;
QString outRightDir;

void MainWindow::preview(int index = 0) {
    QString filter = "*.tif;*.tiff;*.dpx";
    QDir inLeftDir;
    QFileInfo fileInfo;
    QString file;
    QImage image;

    fileInfo = inLeftList.at(index);
    file = fileInfo.absoluteFilePath();
    if (!image.load(file)) {
        ui->previewLabel->setText("Image preview not supported for this file");
    } else {
        QPixmap pixmap(QPixmap::fromImage(image).scaled(ui->previewLabel->size(), Qt::KeepAspectRatio));
        ui->previewLabel->setPixmap(pixmap);
    }
}

void j2kEncode(QStringList pair) {
    convert_to_j2k(context,pair.at(0).toAscii().data(),pair.at(1).toAscii().data(), NULL);
}

void MainWindow::j2kConvert() {
     int threadCount = 0;
     QString inFile;
     QString outFile;
     QList<QStringList> list; 

    // reset iterations
    iterations = 0;

    // set thread limit
    QThreadPool::globalInstance()->setMaxThreadCount(ui->threadsSpinBox->value());

    // build conversion list 
    for (int i = ui->startSpinBox->value() - 1; i < ui->endSpinBox->value(); i++) {
        QStringList pair;

        inFile  = inLeftList.at(i).absoluteFilePath();
        outFile = outLeftDir % "/" % inLeftList.at(i).completeBaseName() % ".j2c";
        pair << inFile << outFile ;

        if (!QFileInfo(outFile).exists() || context->no_overwrite == 0) {
            list.append(pair);
            iterations++;
        }

        if (context->stereoscopic) {
            pair.clear();
            inFile  = inRightList.at(i).absoluteFilePath();
            outFile = outRightDir % "/" % inRightList.at(i).completeBaseName() % ".j2c";
            pair << inFile << outFile ;

            if (!QFileInfo(outFile).exists() || context->no_overwrite == 0) {
                list.append(pair);
                iterations++;
            }
        }
    }

    threadCount = QThreadPool::globalInstance()->maxThreadCount();
    dJ2kConversion->init(iterations, threadCount);

    // Create a QFutureWatcher and conncect signals and slots.
    QFutureWatcher<void> futureWatcher;
    QObject::connect(dJ2kConversion, SIGNAL(cancel()), &futureWatcher, SLOT(cancel()));
    QObject::connect(&futureWatcher, SIGNAL(progressValueChanged(int)), dJ2kConversion, SLOT(step()));
    QObject::connect(&futureWatcher, SIGNAL(finished()), dJ2kConversion, SLOT(finished()));

    // Start the computation
    futureWatcher.setFuture(QtConcurrent::map(list, j2kEncode));

    // open conversion dialog box
    dJ2kConversion->exec();

    // wait to ensure all threads are finished
    futureWatcher.waitForFinished();

    return;
}

void MainWindow::j2kUpdateEndSpinBox() {
    ui->startSpinBox->setMaximum(ui->endSpinBox->value());
}

void MainWindow::j2kCheckLeftInputFiles() {
    QString filter = "*.tif;*.tiff;*.dpx";
    QDir inLeftDir;

    inLeftDir.cd(ui->inImageLeftEdit->text());
    inLeftDir.setFilter(QDir::Files | QDir::NoSymLinks);
    inLeftDir.setNameFilters(filter.split(';'));
    inLeftDir.setSorting(QDir::Name);
    inLeftList = inLeftDir.entryInfoList();

    if (inLeftList.size() < 1) {
        QMessageBox::warning(this, tr("No image files found"),
                                   tr("No valid image files were found in the selected directory"));
        return;
    }

    if (checkFileSequence(inLeftDir.entryList()) != DCP_SUCCESS) {
       return;
    }


    ui->endSpinBox->setMaximum(inLeftList.size());
    ui->endSpinBox->setValue(inLeftList.size());
    ui->startSpinBox->setMaximum(ui->endSpinBox->value());

    preview();
}

void MainWindow::j2kCheckRightInputFiles() {
    QString filter = "*.tif;*.tiff;*.dpx";
    QDir inRightDir;

    inRightDir.cd(ui->inImageRightEdit->text());
    inRightDir.setFilter(QDir::Files | QDir::NoSymLinks);
    inRightDir.setNameFilters(filter.split(';'));
    inRightDir.setSorting(QDir::Name);
    inRightList = inRightDir.entryInfoList();

    if (inRightList.size() < 1) {
        QMessageBox::warning(this, tr("No image files found"),
                                   tr("No valid image files were found in the selected directory"));
        return;
    }

    if (checkFileSequence(inRightDir.entryList()) != DCP_SUCCESS) {
       return;
    }

    ui->endSpinBox->setValue(inRightList.size());
    ui->startSpinBox->setMaximum(ui->endSpinBox->value());
}

void MainWindow::j2kStart() {
    // create opendcp context
    context = create_opendcp();

    // process options
    context->log_level = 0;
    dcp_log_init(context->log_level, "opendcp.log");

    if (ui->profileComboBox->currentIndex() == 0) {
        context->cinema_profile = DCP_CINEMA2K;
    } else {
        context->cinema_profile = DCP_CINEMA4K;
    }

    if (ui->encoderComboBox->currentIndex() == 0) {
        context->j2k.encoder = J2K_OPENJPEG;
    } else {
        context->j2k.encoder = J2K_KAKADU;
    }

    context->j2k.lut = ui->colorComboBox->currentIndex();
    context->j2k.resize = ui->resizeComboBox->currentIndex();

    if (ui->dpxLogCheckBox->checkState()) {
        context->j2k.dpx = 1;
    } else {
        context->j2k.dpx = 0;
    }

    if (ui->stereoscopicCheckBox->checkState()) {
        context->stereoscopic = 1;
    } else {
        context->stereoscopic = 0;
    }

    if (ui->xyzCheckBox->checkState()) {
        context->j2k.xyz = 1;
    } else {
        context->j2k.xyz = 0;
    }

    if (ui->overwritej2kCB->checkState())
        context->no_overwrite = 0;
    else {
        context->no_overwrite = 1;
    }

    context->frame_rate = ui->frameRateComboBox->currentText().toInt();
    context->bw = ui->bwSlider->value() * 1000000;

    // validate destination directories
    if (context->stereoscopic) {
        outLeftDir = ui->outJ2kLeftEdit->text();
        outRightDir = ui->outJ2kRightEdit->text();

        if (ui->inImageLeftEdit->text().isEmpty() || ui->inImageRightEdit->text().isEmpty()) {
            QMessageBox::warning(this, tr("Source Directories Needed"),tr("Please select source directories"));
            goto Done;
        } else if (ui->outJ2kLeftEdit->text().isEmpty() || ui->outJ2kRightEdit->text().isEmpty()) {
            QMessageBox::warning(this, tr("Destination Directories Needed"),tr("Please select destination directories"));
            goto Done;
        }
    } else {
        outLeftDir = ui->outJ2kLeftEdit->text();

        if (ui->inImageLeftEdit->text().isEmpty()) {
            QMessageBox::warning(this, tr("Source Directory Needed"),tr("Please select a source directory"));
            goto Done;
        } else if (ui->outJ2kLeftEdit->text().isEmpty()) {
            QMessageBox::warning(this, tr("Destination Directory Needed"),tr("Please select a destination directory"));
            goto Done;
        }
    }

    // check left and right files are equal
    if (context->stereoscopic) {
        if (inLeftList.size() != inRightList.size()) {
            QMessageBox::critical(this, tr("File Count Mismatch"),
                                 tr("The left and right image directories have different file counts. They must be the same. Please fix and try again."));
            goto Done;
        }
    }

    // check images
    if (context->j2k.resize == SAMPLE_NONE) {
        if (check_image_compliance(context->cinema_profile, NULL, inLeftList.at(0).absoluteFilePath().toAscii().data()) != DCP_SUCCESS) {
            QMessageBox::warning(this, tr("Invalid DCI Resolution"),
                                 tr("Images are not DCI compliant, select DCI resize to automatically resize or supply DCI compliant images"));
            return;
        }
    }

    j2kConvert();

Done:
    delete_opendcp(context);
}
