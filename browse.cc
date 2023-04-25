#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/un.h>
#include <sys/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>
#include <unistd.h>
#include <termios.h>

#include "debug.h"
#include "ansicurs.h"
#include "screen.h"
#include "config.h"
/*
#include "asn1err.h"
#include "asn1name.h"
#include "asn1stab.h"
*/
#include "smi.h"
#include "mibtree.h"
#include "berobj.h"
#include "connect.h"
#include "clsrv.h"
#include "snmpman.h"

//PersistentConfig *config = 0;
FILE *debugfp = 0;
char *ApplicationName = "OMS MIB Browser";
char *ApplicationVersion = "1.006";
clSNMPManager *snmpman = 0;

/**************************************************************/

class clSettingsScreen : public FormScreen
{
protected:
    virtual Screen *HandleFKey(int ch, int &done)
    {
	done = 1;
	return 0;
    }
public:
    clSettingsScreen(Screen *parent_in, char *host_in, char *community_in,
			char *retries_in, char *timeout_in)
	: FormScreen(parent_in, "Settings", "Change settings and press Done.",
		4, 4)
    {
	AddLabel(3, 2, "Host:");
	AddField(new DataField(3, 14, 30));
	((DataField*)fields[0])->SetValue(host_in);
	AddLabel(5, 2, "Community:");
	AddField(new DataField(5, 14, 30));
	((DataField*)fields[1])->SetValue(community_in);
	AddLabel(7, 2, "Retries:");
	AddField(new NumberField(7, 14, 30, 0, 10));
	((DataField*)fields[2])->SetValue(retries_in);
	AddLabel(9, 2, "Timeout:");
	AddField(new NumberField(9, 14, 30, 0, 300));
	((DataField*)fields[3])->SetValue(timeout_in);
	fkeys = new FKeySet8by2("  Done", 0, 0, 0, 0, 0, 0, 0);
    }
    char *Host()
    {
	return ((DataField*)fields[0])->Value();
    }
    char *Community()
    {
	return ((DataField*)fields[1])->Value();
    }
    char *RetryCount()
    {
	return ((DataField*)fields[2])->Value();
    }
    char *Timeout()
    {
	return ((DataField*)fields[3])->Value();
    }
    virtual ~clSettingsScreen()
    { }
};

class clDescScreen : public Screen
{
protected:
    virtual Screen *HandleFKey(int ch, int &done);
public:
    clDescScreen(Screen *parent_in, clMIBTreeNode *node_in);
    virtual ~clDescScreen()
    { }
};

class clBrowseScreen : public PageableScreen
{
    int cnt;
    Menu msgs;
    clMIBTreeNode *node;
protected:
    virtual void GetItems();
    virtual int PrevPage();
    virtual int NextPage();
    virtual Screen *HandleKey(int ch, int &done);
    virtual Screen *HandleFKey(int ch, int &done);
    virtual void Paint();
public:
    clBrowseScreen(Screen *parent_in, clMIBTreeNode *node_in);
    virtual ~clBrowseScreen()
    { }
};


class clBrowseRowsScreen : public PageableScreen
{
    int cnt;
    Menu msgs;
    char *tblid;
    char **vals;
    clMIBTreeNode *node;
protected:
    virtual void GetItems();
    virtual int PrevPage();
    virtual int NextPage();
    virtual Screen *HandleKey(int ch, int &done);
    virtual Screen *HandleFKey(int ch, int &done);
    virtual void Paint();
    void FreeValues();
public:
    clBrowseRowsScreen(Screen *parent_in, clMIBTreeNode *node_in);
    virtual ~clBrowseRowsScreen();
};

//------------------------------------------------------------------------

void 
clBrowseScreen::GetItems()
{
    clMIBTreeNode *c = node->FindChildNum(pagenum*msglines), *fc = c;
    for (int m = 0; m < msglines && c != 0; m++, c = c->NextPeer())
    {
	char buf[80];
	if (c->HasInstance())
	{
	    char *n = c->NumericObjectID();
	    char *v = snmpman->GetValue(n);
	    sprintf(buf, "  %-30s%-40.40s", (char *)c->Name(), v);
	    delete [] n;
	    delete [] v;
	}
	else sprintf(buf, "  %-70s", (char *)c->Name());
	msgs.AddEntry(m, 5+m, 4, buf);
    }
    while (m < msglines)
	msgs.DelEntry(m++);
    msgs.SetCurrent(0);
    fkeys->Enable(0, F2_PRI, fc && fc->FirstChild());
    fkeys->Enable(0, F3_PRI, fc && fc->Description());
    enAccess acc = (fc && fc->HasInstance()) ?
			((clObjectTypNode*)fc)->Access() : ACC_UNACCESS;
    fkeys->Enable(0, F7_PRI, (acc == ACC_READWRITE || acc == ACC_WRITEONLY));
    fkeys->Paint();
}

int 
clBrowseScreen::PrevPage()
{
    return msglines;
}

int 
clBrowseScreen::NextPage()
{
    int rtn = cnt - (pagenum+1)*msglines;
    return (rtn<0) ? 0 : rtn;
}

Screen *
clBrowseScreen::HandleKey(int ch, int &done)
{
    if (ch == KB_DOWN)
	msgs.Down(0);
    else if (ch == KB_UP)
	msgs.Up(0);
    else if (ch != '\n')
	return PageableScreen::HandleKey(ch, done);
    clMIBTreeNode *e = node->FindChildNum(pagenum*msglines+msgs.Current());
    if (ch == '\n')
	return (e && e->FirstChild()) ? HandleFKey(F2_PRI, done) : 0;
    msgs.Paint();
    fkeys->Enable(0, F2_PRI, e && e->FirstChild());
    fkeys->Enable(0, F3_PRI, e && e->Description());
    enAccess acc = (e && e->HasInstance()) ?
			((clObjectTypNode*)e)->Access() : ACC_UNACCESS;
    fkeys->Enable(0, F7_PRI, (acc == ACC_READWRITE || acc == ACC_WRITEONLY));
    fkeys->Paint();
    return 0;
}

Screen *
clBrowseScreen::HandleFKey(int ch, int &done)
{
    done = 0;
    clMIBTreeNode *e = node->FindChildNum(pagenum*msglines+msgs.Current());
    switch(ch)
    {
    case F1_PRI:
        done = 1;
	break;
    case F2_PRI:
        assert(e && e->FirstChild());
	if (e->IsTable())
	    return new clBrowseRowsScreen(this, e);
	else
	    return new clBrowseScreen(this, e);
	break;
    case F3_PRI: // describe
	assert(e && e->Description());
	return new clDescScreen(this, e);
    case F7_PRI: // set
        PopupPrompt *pp = new PopupPrompt(this, "Enter value and press ENTER",
				0, " Cancel", 0, 0);
	char val[80], *err = 0;
	strncpy(val, msgs.GetEntry()+32, 79);
	val[79] = 0;
	for (int i = strlen(val)-1; i>=0 && val[i] == ' '; i--)
	    val[i] = 0;
	// IF THIS IS AN ENUM, MAKE A TOGGLE BOX OR LIST BOX!!
	if (pp->Run(val) == '\n')
	{
	    err = snmpman->Set(e, val);
	    if (err == 0) GetItems();
	}
	delete pp;
	ClearScreen();
	Paint();
	if (err)
	{
	    char buf[82];
	    sprintf(buf, "Set request failed: %-60.60s", err);
	    DrawTitle(buf);
	}
	delete [] err;
	break;
    case F8_PRI:
	clSettingsScreen *s = new clSettingsScreen(this,
					config->Get("host"),
					config->Get("community"),
					config->Get("retries"),
					config->Get("timeout"));
	s->Run();
	config->Set("host", s->Host());
	snmpman->SetHost(s->Host());
	config->Set("community", s->Community());
	snmpman->SetCommunity(s->Community());
	config->Set("retries", s->RetryCount());
	snmpman->SetRetryCount(atoi(s->RetryCount()));
	config->Set("timeout", s->Timeout());
	snmpman->SetTimeout(atoi(s->Timeout()));
	delete s;
	GetItems();
	ClearScreen();
	Paint();
	break;
    }
    return 0;
}

void
clBrowseScreen::Paint()
{
    PageableScreen::Paint();
    if (cnt) msgs.Paint();
    is_lastpage = (cnt-pagenum*msglines) <=msglines;
    ShowPagePrompt(pagenum, is_lastpage);
}

clBrowseScreen::clBrowseScreen(Screen *parent_in, clMIBTreeNode *node_in)
    : PageableScreen(parent_in, /*(char*)node_in->Name(),*/config->Get("host"),
			"Select an entry and an action", 2, 0, 14),
      node(node_in), msgs(14)
{
    AddLabel(3, 2, "Node:");
    AddLabel(3, 8, (char*)node->ObjectID());
    cnt = node->NumChildren();
    fkeys = new FKeySet8by2("  Up", "  Down", " Desc", 0, 0, 0, "  Set", " Config");
}

//-----------------------------------------------------------------------

void 
clBrowseRowsScreen::GetItems()
{
    clMIBTreeNode *c = node->FirstChild()->FindChildNum(pagenum*msglines);
    for (int m = 0, i = pagenum*msglines;
	 m < msglines && i < cnt;
	 m++, i++)
    {
	char buf[80];
	sprintf(buf, "  %-30s%-40.40s", (char *)c->Name(), vals[i]);
	msgs.AddEntry(m, 5+m, 4, buf);
	c = c->NextPeer();
    }
    while (m < msglines)
	msgs.DelEntry(m++);
    msgs.SetCurrent(0);
}

int 
clBrowseRowsScreen::PrevPage()
{
    return msglines;
}

int 
clBrowseRowsScreen::NextPage()
{
    int rtn = cnt - (pagenum+1)*msglines;
    return (rtn<0) ? 0 : rtn;
}

Screen *
clBrowseRowsScreen::HandleKey(int ch, int &done)
{
    if (ch == KB_DOWN)
	msgs.Down(0);
    else if (ch == KB_UP)
	msgs.Up(0);
    else return PageableScreen::HandleKey(ch, done);
    msgs.Paint();
    return 0;
}

Screen *
clBrowseRowsScreen::HandleFKey(int ch, int &done)
{
    done = 0;
    switch(ch)
    {
    case F1_PRI:
        done = 1;
	break;
    case F2_PRI: // next row
	FreeValues();
        cnt = snmpman->GetNext(vals);
	if (cnt < 0)
	{
	    SetLabel(2, 4, 2, "No response from host");
	    cnt = 0;
	}
	else SetLabel(2, 4, 2, cnt ? "" : "No more entries in table");
        fkeys->Enable(0, F2_PRI, cnt);
        fkeys->Enable(0, F3_PRI, cnt);
	GetItems();
	ClearScreen();
	Paint();
	break;
    case F3_PRI: // describe
        clMIBTreeNode *e = node->FirstChild()->FindChildNum(pagenum*msglines+msgs.Current());
	if (e && e->Description())
	    return new clDescScreen(this, e);
	break;
    }
    return 0;
}

void
clBrowseRowsScreen::Paint()
{
    PageableScreen::Paint();
    if (cnt) msgs.Paint();
    is_lastpage = (cnt-pagenum*msglines) <=msglines;
    ShowPagePrompt(pagenum, is_lastpage);
}

clBrowseRowsScreen::clBrowseRowsScreen(Screen *parent_in, clMIBTreeNode *node_in)
    : PageableScreen(parent_in, /*(char*)node_in->Name(),*/config->Get("host"),
			"Select an action", 3, 0, 14),
      node(node_in), msgs(14)
{
    AddLabel(3, 2, "Table:");
    AddLabel(3, 8, (char*)node->ObjectID());
    cnt = node->NumChildren();
    fkeys = new FKeySet8by2("  Up", "  Next\n  Row", " Desc", 0, 0, 0, 0, 0);
    tblid = node->NumericObjectID();
    snmpman->SelectTable((char*)tblid);
    cnt = snmpman->GetNext(vals);
    AddLabel(4, 2, cnt ? "": "No more entries in table");
    fkeys->Enable(0, F2_PRI, cnt);
    fkeys->Enable(0, F3_PRI, cnt);
}

void clBrowseRowsScreen::FreeValues()
{
    for (int i = 0; i < cnt; i++)
	delete [] vals[i];
    delete [] vals;
}

clBrowseRowsScreen::~clBrowseRowsScreen()
{
    FreeValues();
    delete [] tblid;
}

//-----------------------------------------------------------------------

clDescScreen::clDescScreen(Screen *parent_in, clMIBTreeNode *node_in)
    : Screen(parent_in, (char*)node_in->Name(), "Press Cancel when done.",16)
{
    char *d = (char *)node_in->Description(), *s;
    for (int l = 0; l < 16 && d; l++)
    {
	char buf[80];
	s = strchr(d, '\n');
	if (s == 0)
	    strcpy(buf, d);
	else
	{
	    strncpy(buf, d, s-d);
	    buf[s-d] = 0;
	    s++;
	}
	AddLabel(3+l, 2, buf);
	d = s;
    }
    for (; l < 16; l++)
	AddLabel(3+l, 2, "");
    fkeys = new FKeySet8by2(" Cancel",0,0,0,0,0,0,0);
}

Screen *clDescScreen::HandleFKey(int ch, int &done)
{
    (void)ch;
    done = 1;
    return 0;
}

//---------------------------------------------------------------

void 
ShutDown()
{
    QuitCurses();
    delete config;
    if (debugfp) fclose(debugfp);
}

void useage()
{
    fprintf(stderr, "Useage: browse <MIB>\n");
    exit(-1);
}

int 
main(int argc, char *argv[])
{
    config = new PersistentConfig("omssnmp", "host:community:retries:timeout");
    for (int i = 1; i < (argc-1); i++)
    {
    	if (argv[i][0]=='-' && argv[i][2]==0)
    	    switch(argv[i][1])
    	    {
    	    default:
    		useage();
    	    }
    	else useage();
    }
    if (i != (argc-1)) useage();
    snmpman = new clSNMPManager();
    if (snmpman->LoadMIBs(argv[i]) == 0)
    {
        StartCurses();
        SetClip(-3, 0); // write-protect bottom lines
	char *hs = config->Get("host");
	if (hs[0] == 0) hs = "127.0.0.1";
	char *cm = config->Get("community");
	if (cm[0] == 0) cm = "public";
	char *rt = config->Get("retries");
	if (rt[0] == 0) rt = "1";
	char *tm = config->Get("timeout");
	if (tm[0] == 0) tm = "10";
        clSettingsScreen *s = new clSettingsScreen(0, hs, cm, rt, tm);
	s->Run();
	config->Set("host", s->Host());
	config->Set("community", s->Community());
	config->Set("retries", s->RetryCount());
	config->Set("timeout", s->Timeout());
	snmpman->SetHost(s->Host());
	snmpman->SetCommunity(s->Community());
	snmpman->SetRetryCount(atoi(s->RetryCount()));
	snmpman->SetTimeout(atoi(s->Timeout()));
	delete s;
	Screen *bs = new clBrowseScreen(0, snmpman->MIBRoot());
        int rtn = bs->Run();
        bs->DrawTitle("Terminating...");
        delete bs;
        ShutDown();
        return rtn;
    }
    else fprintf(stderr, "Failed to compile MIB - aborting...\n");
    return -1;
}


