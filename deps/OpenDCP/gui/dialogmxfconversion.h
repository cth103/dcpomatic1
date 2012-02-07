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

#ifndef DIALOGMXFCONVERSION_H
#define DIALOGMXFCONVERSION_H

#include "ui_dialogmxfconversion.h"

class DialogMxfConversion : public QDialog, private Ui::DialogMxfConversion {
    Q_OBJECT

public:
    DialogMxfConversion(QWidget *parent = 0);
    void init(int imageCount, QString outputFile);
    void finished(int status);

private:
    int totalCount;
    int currentCount;
    int cancelled;
    int done;
    QString mxfOutputFile;

signals:
    void cancel();

public slots:
    void setButtons(int);
    void step();

private slots:
    void abort();
};

#endif // DIALOGMXFCONVERSION_H
