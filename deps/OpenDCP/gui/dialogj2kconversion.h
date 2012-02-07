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

#ifndef DIALOGJ2KCONVERSION_H
#define DIALOGJ2KCONVERSION_H

#include "ui_dialogj2kconversion.h"

class DialogJ2kConversion : public QDialog, private Ui::DialogJ2kConversion {
    Q_OBJECT

public:
    DialogJ2kConversion(QWidget *parent = 0);
    void init(int imageCount, int threadCount);

private:
    int totalCount;
    int currentCount;
    int cancelled;
    int done;

signals:
    void cancel();

public slots:
    void setButtons(int);
    void step();
    void finished();

private slots:
    void abort();
};

#endif // DIALOGJ2KCONVERSION_H
