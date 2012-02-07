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
#include "generatetitle.h"
#include "dialogj2kconversion.h"
#include "dialogmxfconversion.h"
#include "mxf-writer.h"
#include "opendcp.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    generateTitle   = new GenerateTitle(this);
    dJ2kConversion  = new DialogJ2kConversion();
    dMxfConversion  = new DialogMxfConversion();
    mxfWriterThread = new MxfWriter(this);

    connectSlots();
    setInitialUiState();
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::connectSlots()
{
    j2kConnectSlots();
    mxfConnectSlots();
    connectXmlSlots();

    // set initial directory
    lastDir = QString::null;

    // connect tool button to line edits
    connect(&signalMapper, SIGNAL(mapped(QWidget*)),this, SLOT(getPath(QWidget*)));
}

void MainWindow::setInitialUiState()
{
    ui->mxfSoundRadio2->setChecked(1);

    // set initial screen indexes
    j2kSetStereoscopicState();
    mxfSetStereoscopicState();
    mxfSetHVState();
    mxfSetSoundState();
    mxfSetSlideState();

    // Check For Kakadu
    QProcess *kdu;
    kdu = new QProcess(this);
    int exitCode = kdu->execute("kdu_compress", QStringList() << "-version");
    if (exitCode) {
        int value = ui->encoderComboBox->findText("Kakadu");
        ui->encoderComboBox->removeItem(value);
    }
    delete kdu;

    // Set thread count
#ifdef Q_WS_WIN
    ui->threadsSpinBox->setMaximum(6);
#endif
    ui->threadsSpinBox->setMaximum(QThreadPool::globalInstance()->maxThreadCount());
    ui->threadsSpinBox->setValue(QThread::idealThreadCount());

    ui->mxfSourceTypeComboBox->setCurrentIndex(0);
    ui->mxfInputStack->setCurrentIndex(0);
    ui->mxfTypeComboBox->setCurrentIndex(1);
    ui->tabWidget->setCurrentIndex(0);

    // File Menu
    ui->menuFile->addSeparator();
    ui->menuFile->addAction(tr("E&xit"), this, SLOT(close()),QKeySequence(tr("Ctrl+Q")));

    // Help Menu
    ui->menuHelp->addSeparator();
    ui->menuHelp->addAction(tr("About OpenDCP"), this, SLOT(about()));
}

void MainWindow::getPath(QWidget *w)
{
    QString path;
    QString filter;

    if (w->objectName().contains("picture") ||
        w->objectName().contains("Image")   ||
        w->objectName().contains("J2k")) {
        if (w->objectName().contains("picture") && ui->mxfSourceTypeComboBox->currentIndex() == 1) {
            filter = "*.m2v";
            path = QFileDialog::getOpenFileName(this, tr("Choose an mpeg2 file"),lastDir);
        } else {
            path = QFileDialog::getExistingDirectory(this, tr("Choose a directory of images"),lastDir);
        }
    } else if (w->objectName().contains("subIn")) {
        filter = "*.xml";
        path = QFileDialog::getOpenFileName(this, tr("DCDM Subtitle File"),lastDir);
    } else if (w->objectName().contains("MxfOut")) {
        filter = "*.mxf";
        path = QFileDialog::getSaveFileName(this, tr("Save MXF as"),lastDir,filter);
    } else {
        filter = "*.wav";
        path = QFileDialog::getOpenFileName(this, tr("Choose a file to open"),lastDir,filter);
    }

    w->setProperty("text", path);
    lastDir = path;
}

int findSeqOffset(const char str1[], const char str2[])
{
    int i = 0;
    while(1) {
        if(str1[i] != str2[i])
                return i;
        if(str1[i] == '\0' || str2[i] == '\0')
                return 0;
        i++;
    }
}

int MainWindow::checkSequential(const char str1[], const char str2[])
{
    long x,y;
    int  i;
    int  offset = 0;

    if (strlen(str1) != strlen(str2)) {
        return STRING_LENGTH_NOTEQUAL;
    }

    for (i = 0; i < (int) strlen(str1); i++) {
        if (str1[i] != str2[i]) {
            offset = i;
            break;
        }
    }
 
    x = strtol(str1+offset,NULL,10);
    y = strtol(str2+offset,NULL,10);

    if ((y - x) == 1) {
        return DCP_SUCCESS;
    } else {
        return STRING_NOTSEQUENTIAL;
    }
}

/* check if filelist is sequential */
int MainWindow::checkFileSequence(QStringList list)
{
    QString msg;
    int     sequential = DCP_SUCCESS;
    int     x = 0;

    if (list.size() <= 1) {
        return DCP_SUCCESS;
    }

    for (x = 0; (x < (list.size()-1)) && (sequential == DCP_SUCCESS); x++) {
        QFileInfo a(list.at(x));
        QFileInfo b(list.at(x+1));
        sequential = checkSequential((char *)a.completeBaseName().toStdString().c_str(),
                                     (char *)b.completeBaseName().toStdString().c_str());
    }

    if (sequential == STRING_LENGTH_NOTEQUAL) {
        QMessageBox::critical(this, tr("File Sequence Check Error"), tr("All filenames must be of the same length."));
        return DCP_ERROR;
    }

    if (sequential == STRING_NOTSEQUENTIAL) {
        QTextStream(&msg) << "File list does not appear to be sequential between " << list.at(x-1);
        QTextStream(&msg) << " and " << list.at(x) << ". Do you wish to continue?";
        if (QMessageBox::question(this, tr("File Sequence Mismatch"), msg, QMessageBox::No,QMessageBox::Yes) == QMessageBox::No) {
            return DCP_ERROR;
        } else {
            return DCP_NO_ERROR;
        }
    }

    if (sequential == DCP_SUCCESS) {
        return DCP_SUCCESS;
    } else {
        return DCP_ERROR;
    }
}

void MainWindow::about()
{
    QString msg;
    QTextStream(&msg) << OPENDCP_NAME << " Version " << OPENDCP_VERSION << "\n\n";
    QTextStream(&msg) << OPENDCP_COPYRIGHT << "\n\n" << OPENDCP_LICENSE << "\n\n" << OPENDCP_WEBSITE << "\n\n";
    QMessageBox::about(this, "About OpenDCP",msg);
}
