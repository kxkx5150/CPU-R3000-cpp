#include "cdriso.h"
#include <sys/time.h>
#include "mem.h"
#include "plugins.h"
#include "../plugins/dfsound/spu.h"


CDRISO::CDRISO(PCSX *_pcsx)
{
    pcsx = _pcsx;
}
unsigned int CDRISO::msf2sec(char *msf)
{
    return ((msf[0] * 60 + msf[1]) * 75) + msf[2];
}
void CDRISO::sec2msf(unsigned int s, char *msf)
{
    msf[0] = s / 75 / 60;
    s      = s - msf[0] * 75 * 60;
    msf[1] = s / 75;
    s      = s - msf[1] * 75;
    msf[2] = s;
}
void CDRISO::tok2msf(char *time, char *msf)
{
    char *token;
    token = strtok(time, ":");
    if (token) {
        msf[0] = atoi(token);
    } else {
        msf[0] = 0;
    }
    token = strtok(NULL, ":");
    if (token) {
        msf[1] = atoi(token);
    } else {
        msf[1] = 0;
    }
    token = strtok(NULL, ":");
    if (token) {
        msf[2] = atoi(token);
    } else {
        msf[2] = 0;
    }
}
long CDRISO::GetTickCount(void)
{
    time_t         initial_time = 0;
    struct timeval now;
    gettimeofday(&now, NULL);
    if (initial_time == 0) {
        initial_time = now.tv_sec;
    }
    return (now.tv_sec - initial_time) * 1000L + now.tv_usec / 1000L;
}
void *CDRISO::playthread(void *param)
{
    return NULL;
}
void CDRISO::stopCDDA()
{
}
void CDRISO::startCDDA(unsigned int offset)
{
    printf("startCDDA\n");
}
int CDRISO::parsetoc(const char *isofile)
{
    char         tocname[MAXPATHLEN];
    FILE        *fi;
    char         linebuf[256], dummy[256], name[256];
    char        *token;
    char         time[20], time2[20];
    unsigned int t;
    numtracks = 0;
    strncpy(tocname, isofile, sizeof(tocname));
    tocname[MAXPATHLEN - 1] = '\0';
    if (strlen(tocname) >= 4) {
        strcpy(tocname + strlen(tocname) - 4, ".toc");
    } else {
        return -1;
    }
    if ((fi = fopen(tocname, "r")) == NULL) {
        strcpy(tocname + strlen(tocname) - 4, ".cue");
        if ((fi = fopen(tocname, "r")) == NULL) {
            strcpy(tocname, isofile);
            t = strlen(tocname);
            if (t >= 8 && strcmp(tocname + t - 8, ".toc.bin") == 0) {
                tocname[t - 4] = '\0';
                if ((fi = fopen(tocname, "r")) == NULL) {
                    return -1;
                }
            } else {
                return -1;
            }
        }
    }
    memset(&ti, 0, sizeof(ti));
    cddaBigEndian = TRUE;
    while (fgets(linebuf, sizeof(linebuf), fi) != NULL) {
        strncpy(dummy, linebuf, sizeof(linebuf));
        token = strtok(dummy, " ");
        if (token == NULL)
            continue;
        if (!strcmp(token, "TRACK")) {
            token = strtok(NULL, " ");
            numtracks++;
            if (!strncmp(token, "MODE2_RAW", 9)) {
                ti[numtracks].type = trackinfo::DATA;
                sec2msf(2 * 75, ti[numtracks].start);
                token = strtok(NULL, " ");
                if (token != NULL && !strncmp(token, "RW_RAW", 6)) {
                    subChanMixed = TRUE;
                    subChanRaw   = TRUE;
                }
            } else if (!strncmp(token, "AUDIO", 5)) {
                ti[numtracks].type = trackinfo::CDDA;
            }
        } else if (!strcmp(token, "DATAFILE")) {
            if (ti[numtracks].type == trackinfo::CDDA) {
                sscanf(linebuf, "DATAFILE \"%[^\"]\" #%d %8s", name, &t, time2);
                t /= CD_FRAMESIZE_RAW + (subChanMixed ? SUB_FRAMESIZE : 0);
                t += 2 * 75;
                sec2msf(t, (char *)&ti[numtracks].start);
                tok2msf((char *)&time2, (char *)&ti[numtracks].length);
            } else {
                sscanf(linebuf, "DATAFILE \"%[^\"]\" %8s", name, time);
                tok2msf((char *)&time, (char *)&ti[numtracks].length);
            }
        } else if (!strcmp(token, "FILE")) {
            sscanf(linebuf, "FILE \"%[^\"]\" #%d %8s %8s", name, &t, time, time2);
            tok2msf((char *)&time, (char *)&ti[numtracks].start);
            t /= CD_FRAMESIZE_RAW + (subChanMixed ? SUB_FRAMESIZE : 0);
            t += msf2sec(ti[numtracks].start) + 2 * 75;
            sec2msf(t, (char *)&ti[numtracks].start);
            tok2msf((char *)&time2, (char *)&ti[numtracks].length);
        }
    }
    fclose(fi);
    return 0;
}
int CDRISO::parsecue(const char *isofile)
{
    char         cuename[MAXPATHLEN];
    FILE        *fi;
    char        *token;
    char         time[20];
    char        *tmp;
    char         linebuf[256], dummy[256];
    unsigned int t;
    numtracks = 0;
    strncpy(cuename, isofile, sizeof(cuename));
    cuename[MAXPATHLEN - 1] = '\0';
    if (strlen(cuename) >= 4) {
        strcpy(cuename + strlen(cuename) - 4, ".cue");
    } else {
        return -1;
    }
    if ((fi = fopen(cuename, "r")) == NULL) {
        return -1;
    }
    if (fgets(linebuf, sizeof(linebuf), fi) != NULL) {
        if (!strncmp(linebuf, "CD_ROM_XA", 9)) {
            fclose(fi);
            return parsetoc(isofile);
        }
        fseek(fi, 0, SEEK_SET);
    }
    memset(&ti, 0, sizeof(ti));
    while (fgets(linebuf, sizeof(linebuf), fi) != NULL) {
        strncpy(dummy, linebuf, sizeof(linebuf));
        token = strtok(dummy, " ");
        if (token == NULL) {
            continue;
        }
        if (!strcmp(token, "TRACK")) {
            numtracks++;
            if (strstr(linebuf, "AUDIO") != NULL) {
                ti[numtracks].type = trackinfo::CDDA;
            } else if (strstr(linebuf, "MODE1/2352") != NULL || strstr(linebuf, "MODE2/2352") != NULL) {
                ti[numtracks].type = trackinfo::DATA;
            }
        } else if (!strcmp(token, "INDEX")) {
            tmp = strstr(linebuf, "INDEX");
            if (tmp != NULL) {
                tmp += strlen("INDEX") + 3;
                while (*tmp == ' ')
                    tmp++;
                if (*tmp != '\n')
                    sscanf(tmp, "%8s", time);
            }
            tok2msf((char *)&time, (char *)&ti[numtracks].start);
            t = msf2sec(ti[numtracks].start) + 2 * 75;
            sec2msf(t, ti[numtracks].start);
            if (numtracks > 1) {
                t = msf2sec(ti[numtracks].start) - msf2sec(ti[numtracks - 1].start);
                sec2msf(t, ti[numtracks - 1].length);
            }
        }
    }
    fclose(fi);
    if (numtracks >= 1) {
        fseek(cdHandle, 0, SEEK_END);
        t = ftell(cdHandle) / 2352 - msf2sec(ti[numtracks].start) + 2 * 75;
        sec2msf(t, ti[numtracks].length);
    }
    return 0;
}
int CDRISO::parseccd(const char *isofile)
{
    char         ccdname[MAXPATHLEN];
    FILE        *fi;
    char         linebuf[256];
    unsigned int t;
    numtracks = 0;
    strncpy(ccdname, isofile, sizeof(ccdname));
    ccdname[MAXPATHLEN - 1] = '\0';
    if (strlen(ccdname) >= 4) {
        strcpy(ccdname + strlen(ccdname) - 4, ".ccd");
    } else {
        return -1;
    }
    if ((fi = fopen(ccdname, "r")) == NULL) {
        return -1;
    }
    memset(&ti, 0, sizeof(ti));
    while (fgets(linebuf, sizeof(linebuf), fi) != NULL) {
        if (!strncmp(linebuf, "[TRACK", 6)) {
            numtracks++;
        } else if (!strncmp(linebuf, "MODE=", 5)) {
            sscanf(linebuf, "MODE=%d", &t);
            ti[numtracks].type = ((t == 0) ? trackinfo::CDDA : trackinfo::DATA);
        } else if (!strncmp(linebuf, "INDEX 1=", 8)) {
            sscanf(linebuf, "INDEX 1=%d", &t);
            sec2msf(t + 2 * 75, ti[numtracks].start);
            if (numtracks > 1) {
                t = msf2sec(ti[numtracks].start) - msf2sec(ti[numtracks - 1].start);
                sec2msf(t, ti[numtracks - 1].length);
            }
        }
    }
    fclose(fi);
    if (numtracks >= 1) {
        fseek(cdHandle, 0, SEEK_END);
        t = ftell(cdHandle) / 2352 - msf2sec(ti[numtracks].start) + 2 * 75;
        sec2msf(t, ti[numtracks].length);
    }
    return 0;
}
int CDRISO::parsemds(const char *isofile)
{
    char           mdsname[MAXPATHLEN];
    FILE          *fi;
    unsigned int   offset, extra_offset, l, i;
    unsigned short s;
    numtracks = 0;
    strncpy(mdsname, isofile, sizeof(mdsname));
    mdsname[MAXPATHLEN - 1] = '\0';
    if (strlen(mdsname) >= 4) {
        strcpy(mdsname + strlen(mdsname) - 4, ".mds");
    } else {
        return -1;
    }
    if ((fi = fopen(mdsname, "rb")) == NULL) {
        return -1;
    }
    memset(&ti, 0, sizeof(ti));
    fread(&i, 1, sizeof(unsigned int), fi);
    i = SWAP32(i);
    if (i != 0x4944454D) {
        fclose(fi);
        return -1;
    }
    fseek(fi, 0x50, SEEK_SET);
    fread(&offset, 1, sizeof(unsigned int), fi);
    offset = SWAP32(offset);
    offset += 14;
    fseek(fi, offset, SEEK_SET);
    fread(&s, 1, sizeof(unsigned short), fi);
    s         = SWAP16(s);
    numtracks = s;
    fseek(fi, 4, SEEK_CUR);
    fread(&offset, 1, sizeof(unsigned int), fi);
    offset = SWAP32(offset);
    while (1) {
        fseek(fi, offset + 4, SEEK_SET);
        if (fgetc(fi) < 0xA0) {
            break;
        }
        offset += 0x50;
    }
    fseek(fi, offset + 1, SEEK_SET);
    subChanMixed = (fgetc(fi) ? TRUE : FALSE);
    for (i = 1; i <= numtracks; i++) {
        fseek(fi, offset, SEEK_SET);
        ti[i].type = ((fgetc(fi) == 0xA9) ? trackinfo::CDDA : trackinfo::DATA);
        fseek(fi, 8, SEEK_CUR);
        ti[i].start[0] = fgetc(fi);
        ti[i].start[1] = fgetc(fi);
        ti[i].start[2] = fgetc(fi);
        if (i > 1) {
            l = msf2sec(ti[i].start);
            sec2msf(l - 2 * 75, ti[i].start);
        }
        fread(&extra_offset, 1, sizeof(unsigned int), fi);
        extra_offset = SWAP32(extra_offset);
        fseek(fi, extra_offset + 4, SEEK_SET);
        fread(&l, 1, sizeof(unsigned int), fi);
        l = SWAP32(l);
        sec2msf(l, ti[i].length);
        offset += 0x50;
    }
    fclose(fi);
    return 0;
}
int CDRISO::opensubfile(const char *isoname)
{
    char subname[MAXPATHLEN];
    strncpy(subname, isoname, sizeof(subname));
    subname[MAXPATHLEN - 1] = '\0';
    if (strlen(subname) >= 4) {
        strcpy(subname + strlen(subname) - 4, ".sub");
    } else {
        return -1;
    }
    subHandle = fopen(subname, "rb");
    if (subHandle == NULL) {
        return -1;
    }
    return 0;
}
long CDRISO::ISOinit(void)
{
    assert(cdHandle == NULL);
    assert(subHandle == NULL);
    return 0;
}
long CDRISO::ISOshutdown(void)
{
    if (cdHandle != NULL) {
        fclose(cdHandle);
        cdHandle = NULL;
    }
    if (subHandle != NULL) {
        fclose(subHandle);
        subHandle = NULL;
    }
    stopCDDA();
    return 0;
}
void CDRISO::PrintTracks(void)
{
    int i;
    for (i = 1; i <= numtracks; i++) {
        SysPrintf(_("Track %.2d (%s) - Start %.2d:%.2d:%.2d, Length %.2d:%.2d:%.2d\n"), i,
                  (ti[i].type == trackinfo::DATA ? "DATA" : "AUDIO"), ti[i].start[0], ti[i].start[1], ti[i].start[2],
                  ti[i].length[0], ti[i].length[1], ti[i].length[2]);
    }
}
long CDRISO::ISOopen(void)
{
    if (cdHandle != NULL) {
        return 0;
    }
    cdHandle = fopen(pcsx->psxPlugs->GetIsoFile(), "rb");
    if (cdHandle == NULL) {
        return -1;
    }
    SysPrintf(_("Loaded CD Image: %s"), pcsx->psxPlugs->GetIsoFile());
    cddaBigEndian = FALSE;
    subChanMixed  = FALSE;
    subChanRaw    = FALSE;
    if (parsecue(pcsx->psxPlugs->GetIsoFile()) == 0) {
        SysPrintf("[+cue]");
    } else if (parsetoc(pcsx->psxPlugs->GetIsoFile()) == 0) {
        SysPrintf("[+toc]");
    } else if (parseccd(pcsx->psxPlugs->GetIsoFile()) == 0) {
        SysPrintf("[+ccd]");
    } else if (parsemds(pcsx->psxPlugs->GetIsoFile()) == 0) {
        SysPrintf("[+mds]");
    }
    if (!subChanMixed && opensubfile(pcsx->psxPlugs->GetIsoFile()) == 0) {
        SysPrintf("[+sub]");
    }
    SysPrintf(".\n");
    PrintTracks();
    return 0;
}
long CDRISO::ISOclose(void)
{
    if (cdHandle != NULL) {
        fclose(cdHandle);
        cdHandle = NULL;
    }
    if (subHandle != NULL) {
        fclose(subHandle);
        subHandle = NULL;
    }
    stopCDDA();
    return 0;
}
long CDRISO::ISOgetTN(unsigned char *buffer)
{
    buffer[0] = 1;
    if (numtracks > 0) {
        buffer[1] = numtracks;
    } else {
        buffer[1] = 1;
    }
    return 0;
}
long CDRISO::ISOgetTD(unsigned char track, unsigned char *buffer)
{
    if (numtracks > 0 && track <= numtracks) {
        buffer[2] = ti[track].start[0];
        buffer[1] = ti[track].start[1];
        buffer[0] = ti[track].start[2];
    } else {
        buffer[2] = 0;
        buffer[1] = 2;
        buffer[0] = 0;
    }
    return 0;
}
void CDRISO::DecodeRawSubData(void)
{
    unsigned char subQData[12];
    int           i;
    memset(subQData, 0, sizeof(subQData));
    for (i = 0; i < 8 * 12; i++) {
        if (subbuffer[i] & (1 << 6)) {
            subQData[i >> 3] |= (1 << (7 - (i & 7)));
        }
    }
    memcpy(&subbuffer[12], subQData, 12);
}
long CDRISO::ISOreadTrack(unsigned char *time)
{
    if (cdHandle == NULL) {
        return -1;
    }
    if (subChanMixed) {
        fseek(cdHandle, MSF2SECT(btoi(time[0]), btoi(time[1]), btoi(time[2])) * (CD_FRAMESIZE_RAW + SUB_FRAMESIZE) + 12,
              SEEK_SET);
        fread(cdbuffer, 1, DATA_SIZE, cdHandle);
        fread(subbuffer, 1, SUB_FRAMESIZE, cdHandle);
        if (subChanRaw)
            DecodeRawSubData();
    } else {
        fseek(cdHandle, MSF2SECT(btoi(time[0]), btoi(time[1]), btoi(time[2])) * CD_FRAMESIZE_RAW + 12, SEEK_SET);
        fread(cdbuffer, 1, DATA_SIZE, cdHandle);
        if (subHandle != NULL) {
            fseek(subHandle, MSF2SECT(btoi(time[0]), btoi(time[1]), btoi(time[2])) * SUB_FRAMESIZE, SEEK_SET);
            fread(subbuffer, 1, SUB_FRAMESIZE, subHandle);
            if (subChanRaw)
                DecodeRawSubData();
        }
    }
    return 0;
}
unsigned char *CDRISO::ISOgetBuffer(void)
{
    return cdbuffer;
}
long CDRISO::ISOplay(unsigned char *time)
{
    if (&SPUplayCDDAchannel != NULL) {
        if (subChanMixed) {
            startCDDA(MSF2SECT(time[0], time[1], time[2]) * (CD_FRAMESIZE_RAW + SUB_FRAMESIZE));
        } else {
            startCDDA(MSF2SECT(time[0], time[1], time[2]) * CD_FRAMESIZE_RAW);
        }
    }
    return 0;
}
long CDRISO::ISOstop(void)
{
    stopCDDA();
    return 0;
}
unsigned char *CDRISO::ISOgetBufferSub(void)
{
    if (subHandle != NULL || subChanMixed) {
        return subbuffer;
    }
    return NULL;
}
long CDRISO::ISOgetStatus(struct CdrStat *stat)
{
    int sec;
    pcsx->psxPlugs->CDR__getStatus(stat);
    if (playing) {
        stat->Type = 0x02;
        stat->Status |= 0x80;
        sec = cddaCurOffset / CD_FRAMESIZE_RAW;
        sec2msf(sec, (char *)stat->Time);
    } else {
        stat->Type = 0x01;
    }
    return 0;
}
void CDRISO::cdrIsoInit(void)
{
    numtracks = 0;
}
int CDRISO::cdrIsoActive(void)
{
    return (cdHandle != NULL);
}
