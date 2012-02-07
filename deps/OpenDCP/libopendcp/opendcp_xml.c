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

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <libgen.h>
#include <time.h>
#include <inttypes.h>
#include <sys/stat.h>
#include "opendcp.h"
#include <libxml/encoding.h>
#include <libxml/xmlwriter.h>

#define XML_ENCODING "UTF-8"

#ifdef WIN32
char *strsep (char **stringp, const char *delim) {
    register char *s;
    register const char *spanp;
    register int c, sc;
    char *tok;

    if ((s = *stringp) == NULL)
        return (NULL);
    for (tok = s;;) {
        c = *s++;
        spanp = delim;
        do {
            if ((sc = *spanp++) == c) {
                if (c == 0)
                    s = NULL;
                else
                    s[-1] = 0;
                *stringp = s;
                return (tok);
            }
        } while (sc != 0);
    }
}
#endif

char *get_aspect_ratio(char *dimension_string) {
    char *p, *ratio;
    int n, d;
    float a = 0.0;

    ratio = malloc(sizeof(char)*5);    
    p = malloc(strlen(dimension_string)+1);
    strcpy(p,dimension_string);
    n = atoi(strsep(&p," "));
    d = atoi(strsep(&p," "));
    
    if (d>0) {
        a = (n * 1.00) / (d * 1.00);
    }

    if ( a >= 1.77 && a <= 1.78) {
        a = 1.77;
    }

    sprintf(ratio,"%-3.2f",a);

    return(ratio);
}

int write_cpl(opendcp_t *opendcp, cpl_t *cpl) {
    int a,r, rc;
    struct stat st;
    xmlIndentTreeOutput = 1;
    xmlDocPtr        doc; 
    xmlTextWriterPtr xml;

    /* create XML document */
    xml = xmlNewTextWriterDoc(&doc,0);

    /* cpl start */
    rc = xmlTextWriterStartDocument(xml, NULL, XML_ENCODING, NULL);
    if (rc < 0) {
        dcp_log(LOG_ERROR,"xmlTextWriterStartDocument failed");
        return DCP_FATAL;
    }

    xmlTextWriterStartElement(xml, BAD_CAST "CompositionPlaylist");
    xmlTextWriterWriteAttribute(xml, BAD_CAST "xmlns", BAD_CAST NS_CPL[opendcp->ns]);
    if (opendcp->xml_sign) {
        xmlTextWriterWriteAttribute(xml, BAD_CAST "xmlns:dsig", BAD_CAST DS_DSIG);
    }

    /* cpl attributes */
    xmlTextWriterWriteFormatElement(xml, BAD_CAST "Id","%s%s","urn:uuid:",cpl->uuid);
    xmlTextWriterWriteFormatElement(xml, BAD_CAST "AnnotationText","%s",cpl->annotation);
    xmlTextWriterWriteFormatElement(xml, BAD_CAST "IssueDate","%s",cpl->timestamp);
    xmlTextWriterWriteFormatElement(xml, BAD_CAST "Creator","%s",cpl->creator);
    xmlTextWriterWriteFormatElement(xml, BAD_CAST "ContentTitleText","%s",cpl->title);
    xmlTextWriterWriteFormatElement(xml, BAD_CAST "ContentKind","%s",cpl->kind);

    /* content version */
    if (opendcp->ns == XML_NS_SMPTE) {
        xmlTextWriterStartElement(xml, BAD_CAST "ContentVersion"); 
        xmlTextWriterWriteFormatElement(xml, BAD_CAST "Id","%s%s_%s","urn:uri:",cpl->uuid,cpl->timestamp);
        xmlTextWriterWriteFormatElement(xml, BAD_CAST "LabelText","%s_%s",cpl->uuid,cpl->timestamp);
        xmlTextWriterEndElement(xml);
    }

    /* rating */
    xmlTextWriterStartElement(xml, BAD_CAST "RatingList");
    if (strcmp(cpl->rating,"")) {
        xmlTextWriterWriteFormatElement(xml, BAD_CAST "Agency","%s",RATING_AGENCY[1]);
        xmlTextWriterWriteFormatElement(xml, BAD_CAST "Label","%s",cpl->rating);
    }
    xmlTextWriterEndElement(xml);

    /* reel(s) Start */
    xmlTextWriterStartElement(xml, BAD_CAST "ReelList");
    for (r=0;r<cpl->reel_count;r++) {
        reel_t reel = cpl->reel[r];
        xmlTextWriterStartElement(xml, BAD_CAST "Reel");
        xmlTextWriterWriteFormatElement(xml, BAD_CAST "Id","%s%s","urn:uuid:",reel.uuid);
        xmlTextWriterStartElement(xml, BAD_CAST "AssetList");

        /* Asset(s) Start */
        for (a=0;a<cpl->reel[r].asset_count;a++) {
            asset_t asset = cpl->reel[r].asset[a];
            if (asset.essence_class == ACT_PICTURE) {
                if (asset.stereoscopic) {
                    xmlTextWriterStartElement(xml, BAD_CAST "msp-cpl:MainStereoscopicPicture");
                    xmlTextWriterWriteAttribute(xml, BAD_CAST "xmlns:msp-cpl", BAD_CAST NS_CPL_3D[opendcp->ns]);
                } else {
                    xmlTextWriterStartElement(xml, BAD_CAST "MainPicture");
                }
            }
            if (asset.essence_class == ACT_SOUND) {
                xmlTextWriterStartElement(xml, BAD_CAST "MainSound");
            }
            if (asset.essence_class == ACT_TIMED_TEXT) {
                xmlTextWriterStartElement(xml, BAD_CAST "MainSubtitle");
            }

            xmlTextWriterWriteFormatElement(xml, BAD_CAST "Id","%s%s","urn:uuid:",asset.uuid);
            xmlTextWriterWriteFormatElement(xml, BAD_CAST "AnnotationText","%s",asset.annotation);
            xmlTextWriterWriteFormatElement(xml, BAD_CAST "EditRate","%s",asset.edit_rate);
            xmlTextWriterWriteFormatElement(xml, BAD_CAST "IntrinsicDuration","%d",asset.intrinsic_duration);
            xmlTextWriterWriteFormatElement(xml, BAD_CAST "EntryPoint","%d",asset.entry_point);
            xmlTextWriterWriteFormatElement(xml, BAD_CAST "Duration","%d",asset.duration);

            if (asset.essence_class == ACT_PICTURE) {
                xmlTextWriterWriteFormatElement(xml, BAD_CAST "FrameRate","%s",asset.frame_rate);
                if (opendcp->ns == XML_NS_SMPTE) {
                    xmlTextWriterWriteFormatElement(xml, BAD_CAST "ScreenAspectRatio","%s",asset.aspect_ratio);
                } else {
                    xmlTextWriterWriteFormatElement(xml, BAD_CAST "ScreenAspectRatio","%s",get_aspect_ratio(asset.aspect_ratio));
                }
            }

            if ( opendcp->digest_flag ) {
                xmlTextWriterWriteFormatElement(xml, BAD_CAST "Hash","%s",asset.digest);
            }
            
            xmlTextWriterEndElement(xml); /* end asset */
        }
     
        xmlTextWriterEndElement(xml);     /* end assetlist */
        xmlTextWriterEndElement(xml);     /* end reel */
    }
    xmlTextWriterEndElement(xml);         /* end reel list */

#ifdef XMLSEC
    if (opendcp->xml_sign) {
        write_dsig_template(opendcp, xml);
    }
#endif

    xmlTextWriterEndElement(xml);         /* end compositionplaylist */

    rc = xmlTextWriterEndDocument(xml);
    if (rc < 0) {
        dcp_log(LOG_ERROR,"xmlTextWriterEndDocument failed %s",cpl->filename);
        return DCP_FATAL;
    }

    xmlFreeTextWriter(xml);
    xmlSaveFormatFile(cpl->filename, doc, 1);
    xmlFreeDoc(doc);

#ifdef XMLSEC
    /* sign the XML file */
    if (opendcp->xml_sign) {
        xml_sign(opendcp, cpl->filename);
    }
#endif

    /* store CPL file size */
    dcp_log(LOG_INFO,"Writing CPL file info");
    stat(cpl->filename, &st);
    sprintf(cpl->size,"%"PRIu64,st.st_size);
    calculate_digest(cpl->filename,cpl->digest);
    
    return DCP_SUCCESS;
}

int write_cpl_list(opendcp_t *opendcp) {
   int placeholder = 0;
   return placeholder;
}

int write_pkl_list(opendcp_t *opendcp) {
   int placeholder = 0;
   return placeholder;
}

int write_pkl(opendcp_t *opendcp, pkl_t *pkl) {
    int a,r,c,rc;
    struct stat st;
    xmlIndentTreeOutput = 1;
    xmlDocPtr        doc;
    xmlTextWriterPtr xml;

    /* create XML document */
    xml = xmlNewTextWriterDoc(&doc,0);

    /* pkl start */
    rc = xmlTextWriterStartDocument(xml, NULL, XML_ENCODING, NULL);
    if (rc < 0) {
        dcp_log(LOG_ERROR,"xmlTextWriterStartDocument failed");
        return DCP_FATAL;
    }

    xmlTextWriterStartElement(xml, BAD_CAST "PackingList");
    xmlTextWriterWriteAttribute(xml, BAD_CAST "xmlns", BAD_CAST NS_PKL[opendcp->ns]);
    if (opendcp->xml_sign) {
        xmlTextWriterWriteAttribute(xml, BAD_CAST "xmlns:dsig", BAD_CAST DS_DSIG);
    }

    /* cpl attributes */
    xmlTextWriterWriteFormatElement(xml, BAD_CAST "Id","%s%s","urn:uuid:",pkl->uuid);
    xmlTextWriterWriteFormatElement(xml, BAD_CAST "AnnotationText","%s",pkl->annotation);
    xmlTextWriterWriteFormatElement(xml, BAD_CAST "IssueDate","%s",opendcp->xml.timestamp);
    xmlTextWriterWriteFormatElement(xml, BAD_CAST "Issuer","%s",opendcp->xml.issuer);
    xmlTextWriterWriteFormatElement(xml, BAD_CAST "Creator","%s",opendcp->xml.creator);

    dcp_log(LOG_INFO,"CPLS: %d",pkl->cpl_count);

    /* asset(s) Start */
    xmlTextWriterStartElement(xml, BAD_CAST "AssetList");
    for (c=0;c<pkl->cpl_count;c++) {
        cpl_t cpl = pkl->cpl[c];
        dcp_log(LOG_INFO,"REELS: %d",cpl.reel_count);
        for (r=0;r<cpl.reel_count;r++) {
            reel_t reel = cpl.reel[r];

            for (a=0;a<reel.asset_count;a++) {
                asset_t asset = reel.asset[a];
                xmlTextWriterStartElement(xml, BAD_CAST "Asset");
                xmlTextWriterWriteFormatElement(xml, BAD_CAST "Id","%s%s","urn:uuid:",asset.uuid);
                xmlTextWriterWriteFormatElement(xml, BAD_CAST "AnnotationText","%s",asset.annotation);
                xmlTextWriterWriteFormatElement(xml, BAD_CAST "Hash","%s",asset.digest);
                xmlTextWriterWriteFormatElement(xml, BAD_CAST "Size","%s",asset.size);
                if (opendcp->ns == XML_NS_SMPTE) {
                    xmlTextWriterWriteFormatElement(xml, BAD_CAST "Type","%s","application/mxf");
                } else {
                    if (asset.essence_class == ACT_PICTURE) {
                        xmlTextWriterWriteFormatElement(xml, BAD_CAST "Type","%s","application/x-smpte-mxf;asdcpKind=Picture");
                    }
                    if (asset.essence_class == ACT_SOUND) {
                        xmlTextWriterWriteFormatElement(xml, BAD_CAST "Type","%s","application/x-smpte-mxf;asdcpKind=Sound");
                    }
                    if (asset.essence_class == ACT_TIMED_TEXT) {
                        xmlTextWriterWriteFormatElement(xml, BAD_CAST "Type","%s","application/x-smpte-mxf;asdcpKind=Subtitle");
                    }
                }
                xmlTextWriterEndElement(xml);      /* end asset */
            }
        }

        /* cpl */
        xmlTextWriterStartElement(xml, BAD_CAST "Asset");
        xmlTextWriterWriteFormatElement(xml, BAD_CAST "Id","%s%s","urn:uuid:",cpl.uuid);
        xmlTextWriterWriteFormatElement(xml, BAD_CAST "Hash","%s",cpl.digest);
        xmlTextWriterWriteFormatElement(xml, BAD_CAST "Size","%s",cpl.size);
        if (opendcp->ns == XML_NS_SMPTE) {
            xmlTextWriterWriteFormatElement(xml, BAD_CAST "Type","%s","text/xml");
        } else {
            xmlTextWriterWriteFormatElement(xml, BAD_CAST "Type","%s","text/xml;asdcpKind=CPL");
        }
        xmlTextWriterEndElement(xml);      /* end cpl asset */
    }
    xmlTextWriterEndElement(xml);      /* end assetlist */

#ifdef XMLSEC
    if (opendcp->xml_sign) {
        write_dsig_template(opendcp, xml);
    }
#endif

    xmlTextWriterEndElement(xml);      /* end packinglist */

    rc = xmlTextWriterEndDocument(xml);
    if (rc < 0) {
        dcp_log(LOG_ERROR,"xmlTextWriterEndDocument failed %s",pkl->filename);
        return DCP_FATAL;
    }

    xmlFreeTextWriter(xml);
    xmlSaveFormatFile(pkl->filename, doc, 1);
    xmlFreeDoc(doc);

#ifdef XMLSEC
    /* sign the XML file */
    if (opendcp->xml_sign) {
        xml_sign(opendcp, pkl->filename);
    }
#endif

    /* store PKL file size */
    stat(pkl->filename, &st);
    sprintf(pkl->size,"%"PRIu64,st.st_size);

    return DCP_SUCCESS;
}

int write_assetmap(opendcp_t *opendcp) {
    xmlIndentTreeOutput = 1;
    xmlDocPtr        doc;
    xmlTextWriterPtr xml;
    char             filename[MAX_PATH_LENGTH];
    int              a,c,r,rc;
    char             uuid_s[40];
    cpl_t            cpl;
    reel_t           reel;

    dcp_log(LOG_DEBUG,"write_assetmap: labeltype: %d",opendcp->ns);
    if (opendcp->assetmap.filename) {
        if (opendcp->ns == XML_NS_INTEROP) {
            sprintf(filename,"%s","ASSETMAP");
        } else {
            sprintf(filename,"%s","ASSETMAP.xml");
        }
    } else {
        sprintf(filename,"%s",opendcp->assetmap.filename);
    }

    /* generate assetmap UUID */
    uuid_random(uuid_s);

    dcp_log(LOG_INFO,"Writing ASSETMAP file %.256s",filename);

    /* create XML document */
    xml = xmlNewTextWriterDoc(&doc,0);

    /* assetmap XML Start */
    rc = xmlTextWriterStartDocument(xml, NULL, XML_ENCODING, NULL);
    if (rc < 0) {
        dcp_log(LOG_ERROR,"xmlTextWriterStartDocument failed");
        return DCP_FATAL;
    }

    xmlTextWriterStartElement(xml, BAD_CAST "AssetMap");
    xmlTextWriterWriteAttribute(xml, BAD_CAST "xmlns", BAD_CAST NS_AM[opendcp->ns]);

    /* assetmap attributes */
    xmlTextWriterWriteFormatElement(xml, BAD_CAST "Id","%s%s","urn:uuid:",uuid_s);
    xmlTextWriterWriteFormatElement(xml, BAD_CAST "Creator","%s",opendcp->xml.creator);
    xmlTextWriterWriteFormatElement(xml, BAD_CAST "VolumeCount","%d",1);
    xmlTextWriterWriteFormatElement(xml, BAD_CAST "IssueDate","%s",opendcp->xml.timestamp);
    xmlTextWriterWriteFormatElement(xml, BAD_CAST "Issuer","%s",opendcp->xml.issuer);

    xmlTextWriterStartElement(xml, BAD_CAST "AssetList");

    dcp_log(LOG_INFO,"Writing ASSETMAP PKL");

    /* PKL */
    xmlTextWriterStartElement(xml, BAD_CAST "Asset");
    xmlTextWriterWriteFormatElement(xml, BAD_CAST "Id","%s%s","urn:uuid:",opendcp->pkl[0].uuid);
    xmlTextWriterWriteFormatElement(xml, BAD_CAST "PackingList","%s","true");
    xmlTextWriterStartElement(xml, BAD_CAST "ChunkList");
    xmlTextWriterStartElement(xml, BAD_CAST "Chunk");
    xmlTextWriterWriteFormatElement(xml, BAD_CAST "Path","%s",basename(opendcp->pkl[0].filename));
    xmlTextWriterWriteFormatElement(xml, BAD_CAST "VolumeIndex","%d",1);
    xmlTextWriterWriteFormatElement(xml, BAD_CAST "Offset","%d",0);
    xmlTextWriterWriteFormatElement(xml, BAD_CAST "Length","%s",opendcp->pkl[0].size);
    xmlTextWriterEndElement(xml); /* end chunk */
    xmlTextWriterEndElement(xml); /* end chunklist */
    xmlTextWriterEndElement(xml); /* end pkl asset */
  
    dcp_log(LOG_INFO,"Writing ASSETMAP CPLs");

    /* CPL */
    for (c=0;c<opendcp->pkl[0].cpl_count;c++) {
        cpl = opendcp->pkl[0].cpl[c];
        xmlTextWriterStartElement(xml, BAD_CAST "Asset");
        xmlTextWriterWriteFormatElement(xml, BAD_CAST "Id","%s%s","urn:uuid:",cpl.uuid);
        xmlTextWriterStartElement(xml, BAD_CAST "ChunkList");
        xmlTextWriterStartElement(xml, BAD_CAST "Chunk");
        xmlTextWriterWriteFormatElement(xml, BAD_CAST "Path","%s",basename(cpl.filename));
        xmlTextWriterWriteFormatElement(xml, BAD_CAST "VolumeIndex","%d",1);
        xmlTextWriterWriteFormatElement(xml, BAD_CAST "Offset","%d",0);
        xmlTextWriterWriteFormatElement(xml, BAD_CAST "Length","%s",cpl.size);
        xmlTextWriterEndElement(xml); /* end chunk */
        xmlTextWriterEndElement(xml); /* end chunklist */
        xmlTextWriterEndElement(xml); /* end cpl asset */

        /* assets(s) start */
        for (r=0;r<cpl.reel_count;r++) {
            reel = cpl.reel[r];
            for (a=0;a<reel.asset_count;a++) {
                asset_t asset = reel.asset[a];
                xmlTextWriterStartElement(xml, BAD_CAST "Asset");
                xmlTextWriterWriteFormatElement(xml, BAD_CAST "Id","%s%s","urn:uuid:",asset.uuid);
                xmlTextWriterStartElement(xml, BAD_CAST "ChunkList");
                xmlTextWriterStartElement(xml, BAD_CAST "Chunk");
                xmlTextWriterWriteFormatElement(xml, BAD_CAST "Path","%s",basename(asset.filename));
                xmlTextWriterWriteFormatElement(xml, BAD_CAST "VolumeIndex","%d",1);
                xmlTextWriterWriteFormatElement(xml, BAD_CAST "Offset","%d",0);
                xmlTextWriterWriteFormatElement(xml, BAD_CAST "Length","%s",asset.size);
                xmlTextWriterEndElement(xml); /* end chunk */
                xmlTextWriterEndElement(xml); /* end chunklist */
                xmlTextWriterEndElement(xml); /* end cpl asset */
            }
        }
    }

    xmlTextWriterEndElement(xml); /* end assetlist */
    xmlTextWriterEndElement(xml); /* end assetmap */

    rc = xmlTextWriterEndDocument(xml);
    if (rc < 0) {
        dcp_log(LOG_ERROR,"xmlTextWriterEndDocument failed %s",filename);
        return DCP_FATAL;
    }

    xmlFreeTextWriter(xml);
    xmlSaveFormatFile(filename, doc, 1);
    xmlFreeDoc(doc);

    return DCP_SUCCESS;
}

int write_volumeindex(opendcp_t *opendcp) {
    xmlIndentTreeOutput = 1;
    xmlDocPtr        doc;
    xmlTextWriterPtr xml;
    char             filename[MAX_PATH_LENGTH];
    int              rc;

    if(opendcp->volindex.filename) {
        if (opendcp->ns == XML_NS_INTEROP) {
            sprintf(filename,"%s","VOLINDEX");
        } else {
            sprintf(filename,"%s","VOLINDEX.xml");
        }
    } else {
        sprintf(filename,"%s",opendcp->volindex.filename);
    }

    dcp_log(LOG_INFO,"Writing VOLINDEX file %.256s",filename);

    /* create XML document */
    xml = xmlNewTextWriterDoc(&doc,0);

    /* volumeindex XML Start */
    rc = xmlTextWriterStartDocument(xml, NULL, XML_ENCODING, NULL);
    if (rc < 0) {
        dcp_log(LOG_ERROR,"xmlTextWriterStartDocument failed");
        return DCP_FATAL;
    }

    xmlTextWriterStartElement(xml, BAD_CAST "VolumeIndex");
    xmlTextWriterWriteAttribute(xml, BAD_CAST "xmlns", BAD_CAST NS_AM[opendcp->ns]);
    xmlTextWriterWriteFormatElement(xml, BAD_CAST "Index","%d",1);
    xmlTextWriterEndElement(xml); 

    rc = xmlTextWriterEndDocument(xml);
    if (rc < 0) {
        dcp_log(LOG_ERROR,"xmlTextWriterEndDocument failed %s",filename);
        return DCP_FATAL;
    }

    xmlFreeTextWriter(xml);
    xmlSaveFormatFile(filename, doc, 1);
    xmlFreeDoc(doc);

    return DCP_SUCCESS;
}
