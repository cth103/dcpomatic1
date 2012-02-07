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

#include <QDir>
#include "dialogmxfconversion.h"

DialogMxfConversion::DialogMxfConversion(QWidget *parent) : QDialog(parent)
{
    setupUi(this);

    setWindowFlags(Qt::Dialog | Qt::WindowMinimizeButtonHint);

    connect(buttonClose, SIGNAL(clicked()), this, SLOT(close()));
    connect(buttonStop, SIGNAL(clicked()), this, SLOT(abort()));
}

void DialogMxfConversion::init(int imageCount, QString outputFile)
{
    currentCount  = 0;
    done          = 0;
    cancelled     = 0;
    totalCount    = imageCount;
    mxfOutputFile = outputFile;

    progressBar->reset();
    progressBar->setMinimum(0);
    progressBar->setMaximum(totalCount);

    setButtons(1);
}

void DialogMxfConversion::step()
{
    QString labelText;

    if (done == 1 && cancelled == 0) {
        currentCount = totalCount;
    }

    labelText.sprintf("MXF File Creation: %s  [Writing %d of %d]",mxfOutputFile.toAscii().constData(),currentCount,totalCount);
    labelTotal->setText(labelText);
    progressBar->setValue(currentCount);

    if (!done) {
        currentCount++;
    }
}

void DialogMxfConversion::finished(int status)
{
    QString labelText;

    done = 1;
    step();
    if (status) {
        labelText.sprintf("MXF File Creation: Writing %d of %d. MXF file %s created successfully.",currentCount,totalCount,mxfOutputFile.toAscii().constData());
        labelTotal->setText(labelText);
    } else {
        labelText.sprintf("MXF File Creation: Writing %d of %d. MXF file %s creation failed.",currentCount,totalCount,mxfOutputFile.toAscii().constData());
        labelTotal->setText(labelText);
    }
    setButtons(0);
}

void DialogMxfConversion::abort()
{
    setButtons(0);
    cancelled = 1;
    emit cancel();
}

void DialogMxfConversion::setButtons(int state)
{
    if (state == 0) {
        buttonClose->setEnabled(true);
        buttonStop->setEnabled(false);
    } else {
        buttonClose->setEnabled(false);
        buttonStop->setEnabled(true);
    }
}
