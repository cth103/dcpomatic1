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

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QtGui>
#include <opendcp.h>
#include "dialogj2kconversion.h"
#include "dialogmxfconversion.h"
#include "mxf-writer.h"

class GenerateTitle;
class DialogJ2kConversion;

namespace Ui {
    class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public slots:

    void getPath(QWidget *w);
    void setPictureTrack();
    void setSoundTrack();
    void setSubtitleTrack();

    void j2kStart();
    void j2kCheckLeftInputFiles();
    void j2kCheckRightInputFiles();
    void j2kBwSliderUpdate();
    void j2kCinemaProfileUpdate();
    void j2kUpdateEndSpinBox();
    void j2kSetStereoscopicState();

    void mxfStart();
    void mxfSourceTypeUpdate();
    void mxfSetStereoscopicState();
    void mxfSetSlideState();
    void mxfSetHVState();
    void mxfSetSoundState();
    void mxfCreateSubtitle();
    void mxfDone();

    void startDcp();
    void updatePictureDuration();
    void updateSoundDuration();
    void updateSubtitleDuration();

    void getTitle();
    void about();
    void preview(int index);

protected:
    void setInitialUiState();
    void connectSlots();
    void j2kConnectSlots();
    void j2kConvert();
    void mxfConnectSlots();
    void mxfCreatePicture();
    void mxfCreateAudio();

    void connectXmlSlots();
    void showImage(QImage image);
    int  checkFileSequence(QStringList list);
    int  checkSequential(const char str1[], const char str2[]); 

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

private:
    Ui::MainWindow      *ui;
    QSignalMapper       signalMapper;
    GenerateTitle       *generateTitle;
    QString             lastDir;
    DialogJ2kConversion *dJ2kConversion;
    DialogMxfConversion *dMxfConversion;
    MxfWriter           *mxfWriterThread;
};

#endif // MAINWINDOW_H
