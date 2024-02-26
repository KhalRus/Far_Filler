#define _FAR_NO_NAMELESS_UNIONS
#define _FAR_USE_FARFINDDATA
#include "plugin.hpp"

enum FillerLng 
{
  MOK,
  MCancel,
  MTitle,
  MMax,
  MMin,
  MGlobal,
  MWork,
  MCurMax,
  MFilePanReq
};

char PluginRootKey[80];
static struct FarStandardFunctions FSF;
static struct PluginStartupInfo Info;
static struct Options 
{
  int max, min, glob;
} Opt;

extern "C"
{
  void WINAPI _export SetStartupInfo(const struct PluginStartupInfo *Info);
  HANDLE WINAPI _export OpenPlugin(int OpenFrom, int Item);
  void WINAPI _export GetPluginInfo(struct PluginInfo *Info); 
  int WINAPI _export Configure(int ada);
};

bool CheckForEsc(void)
{
  bool EC = false;
  INPUT_RECORD rec;
  static HANDLE hConInp = GetStdHandle(STD_INPUT_HANDLE);
  DWORD ReadCount;
  while (1)
  {
    PeekConsoleInput(hConInp, &rec, 1, &ReadCount);
    if (ReadCount == 0) break;
    ReadConsoleInput(hConInp, &rec, 1, &ReadCount);
    if (rec.EventType == KEY_EVENT)
      if (rec.Event.KeyEvent.wVirtualKeyCode == VK_ESCAPE && rec.Event.KeyEvent.bKeyDown) 
    EC = true;
  }
  return(EC);
}

void WINAPI _export SetStartupInfo(const struct PluginStartupInfo *Info)
{
  HKEY hKey;
  DWORD dw, dwSize = sizeof(DWORD);
  ::Info = *Info;
  Opt.max = 4698;
  Opt.min = 4650;
  Opt.glob = 0;
  FSF = *Info->FSF;
  lstrcpy(PluginRootKey, Info->RootKey);
  lstrcat(PluginRootKey, "\\Filler");
  if (RegOpenKeyEx(HKEY_CURRENT_USER, PluginRootKey, 0, KEY_QUERY_VALUE, &hKey) == ERROR_SUCCESS)
  {
    if (RegQueryValueEx(hKey, "max", NULL, NULL, (unsigned char *)&dw, &dwSize) == ERROR_SUCCESS)
      Opt.max = dw;
    if (RegQueryValueEx(hKey, "min", NULL, NULL, (unsigned char *)&dw, &dwSize) == ERROR_SUCCESS)
      Opt.min = dw;
    if (RegQueryValueEx(hKey, "glob", NULL, NULL, (unsigned char *)&dw, &dwSize) == ERROR_SUCCESS)
      Opt.glob = dw;
    RegCloseKey(hKey);
  }
}

static const char *GetMsg(enum FillerLng MsgId) 
{
  return Info.GetMsg(Info.ModuleNumber, MsgId);
}

void WINAPI _export GetPluginInfo(struct PluginInfo *Info)
{
  Info->StructSize = sizeof(*Info);
  Info->Flags = 0;
  Info->DiskMenuStringsNumber = 0;
  static const char *PluginMenuStrings[] = {GetMsg(MTitle)};
  Info->PluginMenuStrings = PluginMenuStrings;
  Info->PluginConfigStrings = PluginMenuStrings;
  Info->PluginMenuStringsNumber = 1;
  Info->PluginConfigStringsNumber = 1;  
}

HANDLE WINAPI _export OpenPlugin(int, int)
{
  typedef __int64 it;
  const char * Msg[4];
  Msg[0] = GetMsg(MTitle);
  Msg[1] = GetMsg(MWork);
  Msg[2] = "";
  HANDLE hN = GetProcessHeap();
  it lbrd = (it)(Opt.min) * (it)1000000, rbrd = (it)(Opt.max) * (it)(1000000), Scur = 0, Smax = 0, Slast = 0, Sn;
  int i, nmax = 0, ncur = 0, pos = 1, ctr = 0, nost, ost[6];
  if (Opt.glob)
    lbrd = rbrd + 1;

  struct PanelInfo AInfo;
  if (!Info.Control(INVALID_HANDLE_VALUE, FCTL_GETPANELINFO, &AInfo))
    return INVALID_HANDLE_VALUE;
  if (AInfo.PanelType != PTYPE_FILEPANEL) 
  {
    const char *MsgItems[] = {GetMsg(MTitle), GetMsg(MFilePanReq), GetMsg(MOK)};
    Info.Message(Info.ModuleNumber, FMSG_WARNING, NULL, MsgItems, sizeof(MsgItems) / sizeof(MsgItems[0]), 1);
    return INVALID_HANDLE_VALUE;
  }

  int count = AInfo.SelectedItemsNumber;
  if (count < 2)
    return INVALID_HANDLE_VALUE;

  char * tmp1 = (char *)HeapAlloc(hN, NULL, 200);
  char * tmp2 = (char *)HeapAlloc(hN, NULL, 200);
  int * max = (int *)HeapAlloc(hN, NULL, (count + 1) * 4);
  int * cnv = (int *)HeapAlloc(hN, NULL, (count + 1) * 4);
  int * cur = (int *)HeapAlloc(hN, NULL, (count + 1) * 4);
  it * fil = (it *)HeapAlloc(hN, NULL, (count + 1) * 8);

  for (i = 0; i < count; i++)
    fil[i + 1] = (it)(AInfo.SelectedItems[i].FindData.nFileSizeHigh) * (it)(MAXDWORD) + (it)(AInfo.SelectedItems[i].FindData.nFileSizeLow);
  for (i = 0; i < AInfo.ItemsNumber; i++)
    if (AInfo.PanelItems[i].Flags & PPIF_SELECTED)
      cnv[++ctr] = i;

  cur[0] = 0;
  ctr = 0;  
  while (ncur != -1)
  {
    if ((Scur + fil[pos]) <= rbrd)
    {
      Scur += fil[pos]; 
      cur[++ncur] = pos;
      if (Scur > Smax)
      {
        Smax = Scur;
        nmax = ncur;
        for (i = 1; i <= ncur; i++)
          max[i] = cur[i];
        if ((Smax > lbrd) || (nmax == count) || (Smax == rbrd))
          ncur = pos = -1;
      }
    }
    if (pos == count)
    {
      Scur -= fil[cur[ncur]];
      pos = cur[ncur--] + 1;
      if (pos == (count + 1))
      {
        Scur -= fil[cur[ncur]];
        pos = cur[ncur--] + 1;
      }
    }
    else
      pos++;
    // -- отрисовка окна поиска, проверка на Esc
    ctr++;
    if (ctr == 50000000)
    {
      if (Smax > Slast)
      {
        lstrcpy(tmp1, GetMsg(MCurMax));
        Sn = Smax;
        nost = -1;
        while (Sn > 0)
        {
          ost[++nost] = Sn % 1000;
          Sn = Sn / 1000;
        }
        for (i = nost; i >= 0; i--)
        {
          FSF.itoa(ost[i], tmp2, 10);
          lstrcat(tmp1, tmp2);
          if (i != 0)
            lstrcat(tmp1, ",");
        }
        Msg[3] = tmp1;
        Info.Message(Info.ModuleNumber, FMSG_LEFTALIGN, 0, Msg, sizeof(Msg)/sizeof(Msg[0]), 0);
        Slast = Smax;
      }
      ctr = 0;
      if (CheckForEsc())
        ncur = -1;
    }
  }
  for (i = 0; i < AInfo.ItemsNumber; i++)
    AInfo.PanelItems[i].Flags &= (~PPIF_SELECTED);
  for (i = 1; i <= nmax; i++)
    AInfo.PanelItems[cnv[max[i]]].Flags |= PPIF_SELECTED;

  Info.Control(INVALID_HANDLE_VALUE, FCTL_SETSELECTION, &AInfo);
  Info.Control(INVALID_HANDLE_VALUE, FCTL_REDRAWPANEL, NULL);       
  HeapFree(hN, 0, max);
  HeapFree(hN, 0, cur);
  HeapFree(hN, 0, fil);
  HeapFree(hN, 0, tmp1);
  HeapFree(hN, 0, tmp2);
  HeapFree(hN, 0, cnv);
  return(INVALID_HANDLE_VALUE);
}

int WINAPI _export Configure(int)
{
  static struct InitDialogItem 
  {
    unsigned char Type;
    unsigned char X1, Y1, X2, Y2;
    enum FillerLng Data;
    unsigned int Flags;
  } InitItems[] = 
  {
    /*  0 */ { DI_DOUBLEBOX, 2,   1,   36, 11, MTitle,  0},
    /*  1 */ { DI_BUTTON,    0,   10,  0,  0,  MOK,     DIF_CENTERGROUP},
    /*  2 */ { DI_BUTTON,    0,   10,  0,  0,  MCancel, DIF_CENTERGROUP},
    /*  3 */ { DI_CHECKBOX,  4,   7,   0,  0,  MGlobal, 0},
    /*  4 */ { DI_TEXT,      4,   3,   0,  0,  MMax,    0},
    /*  5 */ { DI_TEXT,      4,   5,   0,  0,  MMin,    0},
    /*  6 */ { DI_FIXEDIT,  29,  3,   34,  0,  MMax,    DIF_MASKEDIT},
    /*  7 */ { DI_FIXEDIT,  29,  5,   34,  0,  MMin,    DIF_MASKEDIT}
  };
  struct FarDialogItem DialogItems[sizeof(InitItems) / sizeof(InitItems[0])];
  
  int i;
  for (i = sizeof(InitItems) / sizeof(InitItems[0]) - 1; i >= 0; i--) 
  {
    DialogItems[i].Type  = InitItems[i].Type;
    DialogItems[i].X1    = InitItems[i].X1;
    DialogItems[i].Y1    = InitItems[i].Y1;
    DialogItems[i].X2    = InitItems[i].X2;
    DialogItems[i].Y2    = InitItems[i].Y2;
    DialogItems[i].Focus = FALSE;
    DialogItems[i].Flags = InitItems[i].Flags;
    lstrcpy(DialogItems[i].Data.Data, GetMsg(InitItems[i].Data));
  }
  DialogItems[6].Param.Mask = "999999";
  DialogItems[7].Param.Mask = "999999";
  DialogItems[3].Param.Selected = Opt.glob;
  FSF.itoa(Opt.max, DialogItems[6].Data.Data, 10);
  FSF.itoa(Opt.min, DialogItems[7].Data.Data, 10);
  int ExitCode = Info.Dialog(Info.ModuleNumber,-1,-1,39,13,"Config", DialogItems,(sizeof(DialogItems)/sizeof(DialogItems[0])));
  if (ExitCode == 1)
  {
    Opt.max = FSF.atoi(DialogItems[6].Data.Data);
    Opt.min = FSF.atoi(DialogItems[7].Data.Data);
    Opt.glob = DialogItems[3].Param.Selected;
    HKEY hKey;
    DWORD dw, dw1, dwSize = sizeof(DWORD);
    
    if (RegCreateKeyEx(HKEY_CURRENT_USER, PluginRootKey, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hKey, &dw1) == ERROR_SUCCESS)
    {
      dw = (DWORD)Opt.max;
      RegSetValueEx(hKey, "max", 0, REG_DWORD, (BYTE *)&dw, dwSize);
      dw = (DWORD)Opt.min;
      RegSetValueEx(hKey, "min", 0, REG_DWORD, (BYTE *)&dw, dwSize);
      dw = (DWORD)Opt.glob;
      RegSetValueEx(hKey, "glob", 0, REG_DWORD, (BYTE *)&dw, dwSize);
      RegCloseKey(hKey);
    }
  }
  return true;
}