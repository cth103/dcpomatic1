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

#ifndef __MXF_WRITER_H__
#define __MXF_WRITER_H__

#include <QtGui>
#include <QObject>
#include <string.h>
#include <iostream>
#include "AS_DCP.h"
#include <KM_fileio.h>
#include <KM_prng.h>
#include <KM_memio.h>
#include <KM_util.h>
#include "opendcp.h"
#include "dialogmxfconversion.h"

using namespace ASDCP;
using namespace Kumu;

using namespace std;

typedef struct {
    WriterInfo    info;
    AESEncContext *aes_context;
    HMACContext   *hmac_context;
} writer_info_t;

static const ui32_t FRAME_BUFFER_SIZE = 4 * Kumu::Megabyte;

class MxfWriter : public QThread
{
    Q_OBJECT

public:
    MxfWriter(QObject *parent);
    void reset();
    ~MxfWriter();
    void run();
    void setMxfInputs(opendcp_t *opendcp, QFileInfoList fileList, QString outputFile);
    int  success;

private:
    opendcp_t      *opendcpMxf;
    int            cancelled;
    QFileInfoList  mxfFileList;
    QString        mxfOutputFile;

    Result_t writeMxf();
    Result_t fillWriterInfo(opendcp_t *opendcp, writer_info_t *writer_info);
    Result_t writeJ2kStereoscopicMxf(opendcp_t *opendcp, QFileInfoList inputList, QString outputFile);
    Result_t writeJ2kMxf(opendcp_t *opendcp, QFileInfoList inputList, QString outputFile);
    Result_t writePcmMxf(opendcp_t *opendcp, QFileInfoList inputList, QString outputFile);
    Result_t writeMpeg2Mxf(opendcp_t *opendcp, QFileInfoList inputList, QString outputFile);
    Result_t writeTTMxf(opendcp_t *opendcp, QFileInfoList inputList, QString outputFile);

signals:
    void finished();
    void errorMessage(QString);
    void frameDone();

private slots:

};

#endif // __MXF_WRITER_H__
