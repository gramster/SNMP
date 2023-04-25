#include <stdio.h>
#include <string.h>

int readfields(FILE *fp, char *flds[], int maxflds)
{
    for (int i = 0; i < maxflds; i++)
    {
	delete [] flds[i];
	flds[i] = 0;
    }
    char buff[1024], fld[1024];
    int fi = 0, last = 0;
    fld[0] = 0;
    while (!feof(fp) && !last)
    {
	if (fgets(buff, 1024, fp)==0) break;
	if (buff[0]=='#') continue;
	int l = strlen(buff);
	buff[l-1] = 0;
	if (l>1 && buff[l-2]=='\\')
	    buff[l-2] = 0;
	else last = 1;
	char *s = buff;
	for (;;)
	{
	    char *t = strchr(s, ':');
	    if (t) *t++ = 0;
	    strcat(fld, s);
	    s = t;
	    if (s)
	    {
		flds[fi] = new char [strlen(fld)+1];
		strcpy(flds[fi++], fld);
		fld[0] = 0;
	    }
	    else break;
	}
    }
    if (fld[0])
    {
        flds[fi] = new char [strlen(fld)+1];
        strcpy(flds[fi++], fld);
    }
    return fi;
}

int main()
{
    char *flds[20];
    for (int i = 0; i < 20; i++) flds[i] = 0;
    FILE *fp = fopen("mib.db", "r");
    for (;;)
    {
	int rtn = readfields(fp, flds, 20);
	printf("%d: ", rtn);
	for (int j = 0; j < rtn; j++)
	    printf("<%s> ", flds[j]);
	printf("\n");
	if (rtn == 0) break;
    }
    fclose(fp);
    return 0;
}

