//
// RLP unit for GNOKII (Not ported to LINUX/C++)
//
// 1999 (C) Odinokov Serge. E-mail:  Serge@takas.lt or apskaita@post.omnitel.net
//
// Port to C commenced 20/10/1999 by Hugh
// If you work on this code, please use tabs/indent of 4 characters.
// Thankyou ;)

/* Unless otherwise noted, comments have been added during
   porting and hence may be totally misleading :) */

#include	"misc.h"


	/* User bits portion of RLP frame. */
typedef u8 		UData[24];

bool			Ackn_State;
u8				RemoteMachineState;
u8				VA,VD,VR,VS;

// *FIX*
// zz1,zz2,zz3,zzzz : cardinal;
// xx1,xx2,xx3 : cardinal;
// 
	/* No idea what blocks are and too tired to work it out :) */
#define		Blocks		(int)
	
UData		    UserData;
u8			    CurrentFrameType;
u8				Poll_State;
u8				NextState, CurrentState;
Blocks			outBlock;
UData			inXIDData, outXIDData;

bool			WindowOpen, LRReady, Attach_Req, Conn_Ind, Conn_Conf,
				Conn_Conf_Neg, Conn_Req, Conn_Resp, Conn_Resp_Neg,
    			Data_Ind, Data_Req, DISC_Ind, Error_Ind, Reset_Conf, Reset_Ind,
    			Reset_Req, Reset_Resp, XID_Req, XID_Ind_Contention, XID_Ind;

	/* Prototypes */
void			RLPInit(void);
bool			UEvent(bool x, bool change);
bool			Check_input_PDU(Blocks X, Blocks Y); /* Entry point */
void			MainStateMachine(void);
void			SendData(void);
void			SendSREJ_command(u8 x);
void			Send_XX_or_XX_I__command(u8 X);
void			Send_XX_or_XX_I__response(u8 X);
void			SendFrame(u8 outFT, bool outCR, bool outPF, u8 outNR,
						  u8 outNS, UData outData, bool outDTX);

  /*  Code below this line hasn't been converted and hence is commented
   *  out - allows gcc to be run over what has been done so far to make
   *  sure it builds cleanly...
   *  
   *  
Implementation

 Const
  ZeroData : UData = ($1F,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0);
  Timeout1_limit = 55;
  N2 : byte = 15;
  M = 62;

  {Frame types}
  ft_U_SABM = 0;
  ft_U_UA   = 1;
  ft_U_DISC = 2;
  ft_U_DM   = 3;
  ft_U_NULL = 4;
  ft_U_UI   = 5;
  ft_U_XID  = 6;
  ft_U_TEST = 7;
  ft_S_RR   = 8;
  ft_S_REJ  = 9;
  ft_S_RNR  = 10;
  ft_S_SREJ = 11;
  ft_SplusI_RR   = 12; // ft_S_RR + 4
  ft_SplusI_REJ  = 13;
  ft_SplusI_RNR  = 14;
  ft_SplusI_SREJ = 15;
  ft_bad         = $FF;

  _idle = 0;
  _send = 1;
  _wait = 2;
  _rcvd = 3;
  _ackn = 4;
  _rej  = 5;
  _srej = 6;

  coef :array[0..255] of dword = (
      $00000000, $00D6A776, $00F64557, $0020E221, $00B78115, $00612663, $0041C442, $00976334,
      $00340991, $00E2AEE7, $00C24CC6, $0014EBB0, $00838884, $00552FF2, $0075CDD3, $00A36AA5,
      $00681322, $00BEB454, $009E5675, $0048F103, $00DF9237, $00093541, $0029D760, $00FF7016,
      $005C1AB3, $008ABDC5, $00AA5FE4, $007CF892, $00EB9BA6, $003D3CD0, $001DDEF1, $00CB7987,
      $00D02644, $00068132, $00266313, $00F0C465, $0067A751, $00B10027, $0091E206, $00474570,
      $00E42FD5, $003288A3, $00126A82, $00C4CDF4, $0053AEC0, $008509B6, $00A5EB97, $00734CE1,
      $00B83566, $006E9210, $004E7031, $0098D747, $000FB473, $00D91305, $00F9F124, $002F5652,
      $008C3CF7, $005A9B81, $007A79A0, $00ACDED6, $003BBDE2, $00ED1A94, $00CDF8B5, $001B5FC3,
      $00FB4733, $002DE045, $000D0264, $00DBA512, $004CC626, $009A6150, $00BA8371, $006C2407,
      $00CF4EA2, $0019E9D4, $00390BF5, $00EFAC83, $0078CFB7, $00AE68C1, $008E8AE0, $00582D96,
      $00935411, $0045F367, $00651146, $00B3B630, $0024D504, $00F27272, $00D29053, $00043725,
      $00A75D80, $0071FAF6, $005118D7, $0087BFA1, $0010DC95, $00C67BE3, $00E699C2, $00303EB4,
      $002B6177, $00FDC601, $00DD2420, $000B8356, $009CE062, $004A4714, $006AA535, $00BC0243,
      $001F68E6, $00C9CF90, $00E92DB1, $003F8AC7, $00A8E9F3, $007E4E85, $005EACA4, $00880BD2,
      $00437255, $0095D523, $00B53702, $00639074, $00F4F340, $00225436, $0002B617, $00D41161,
      $00777BC4, $00A1DCB2, $00813E93, $005799E5, $00C0FAD1, $00165DA7, $0036BF86, $00E018F0,
      $00AD85DD, $007B22AB, $005BC08A, $008D67FC, $001A04C8, $00CCA3BE, $00EC419F, $003AE6E9,
      $00998C4C, $004F2B3A, $006FC91B, $00B96E6D, $002E0D59, $00F8AA2F, $00D8480E, $000EEF78,
      $00C596FF, $00133189, $0033D3A8, $00E574DE, $007217EA, $00A4B09C, $008452BD, $0052F5CB,
      $00F19F6E, $00273818, $0007DA39, $00D17D4F, $00461E7B, $0090B90D, $00B05B2C, $0066FC5A,
      $007DA399, $00AB04EF, $008BE6CE, $005D41B8, $00CA228C, $001C85FA, $003C67DB, $00EAC0AD,
      $0049AA08, $009F0D7E, $00BFEF5F, $00694829, $00FE2B1D, $00288C6B, $00086E4A, $00DEC93C,
      $0015B0BB, $00C317CD, $00E3F5EC, $0035529A, $00A231AE, $007496D8, $005474F9, $0082D38F,
      $0021B92A, $00F71E5C, $00D7FC7D, $00015B0B, $0096383F, $00409F49, $00607D68, $00B6DA1E,
      $0056C2EE, $00806598, $00A087B9, $007620CF, $00E143FB, $0037E48D, $001706AC, $00C1A1DA,
      $0062CB7F, $00B46C09, $00948E28, $0042295E, $00D54A6A, $0003ED1C, $00230F3D, $00F5A84B,
      $003ED1CC, $00E876BA, $00C8949B, $001E33ED, $008950D9, $005FF7AF, $007F158E, $00A9B2F8,
      $000AD85D, $00DC7F2B, $00FC9D0A, $002A3A7C, $00BD5948, $006BFE3E, $004B1C1F, $009DBB69,
      $0086E4AA, $005043DC, $0070A1FD, $00A6068B, $003165BF, $00E7C2C9, $00C720E8, $0011879E,
      $00B2ED3B, $00644A4D, $0044A86C, $00920F1A, $00056C2E, $00D3CB58, $00F32979, $00258E0F,
      $00EEF788, $003850FE, $0018B2DF, $00CE15A9, $0059769D, $008FD1EB, $00AF33CA, $007994BC,
      $00DAFE19, $000C596F, $002CBB4E, $00FA1C38, $006D7F0C, $00BBD87A, $009B3A5B, $004D9D2D );

   Type UBlock = Record
                   Data : UData;
                   State : byte;
                 End;

  Var
    Ackn_FBit : boolean;
    C_bit : boolean;
    Data : UData;
    DISC_State: byte;
    DM_FBit : boolean;
    DM_State: boolean;
    DTX_SF: byte;
    DTX_VR: byte;
    NR : byte;
    NS : byte;
    P_F : boolean;
    Poll_Count : byte;
    Poll_xchg : boolean;
    RBlocks : array[0 .. M-1] of UBlock;
    RRReady : boolean;
    SABM_Count : byte;
    SABM_State : byte;
    SBlocks : array[0 .. M-1] of UBlock;
    SF : byte;
    T : byte;
    TEST_R_Data : UData;
    TEST_R_FBit : boolean;
    TEST_R_State : boolean;
    T_RCVR : byte;
    T_TEST : byte;
    T_XID : byte;
    UA_FBit : boolean;
    UA_State : boolean;
    UI_Data : UData;
    UI_State : boolean;
    XID_Count : byte;
    XID_C_Data : UData;
    XID_C_PBit : boolean;
    XID_C_State : byte;
    XID_R_FBit : boolean;
    XID_R_State : byte;
    T_RCVS : array[0..M] of byte;

{==============================================================================}
Function Decr(x: byte): byte;
 Begin
  If x = 0 then Result := M - 1 Else Result := x - 1;
 End;

{==============================================================================}
Function Incr(x: byte): byte;
 Begin
  Result := (x + 1) Mod M;
 End;

{==============================================================================}
Procedure PrepareDataToTransmit(var Slot:byte);
 Var
  i,j : byte;
 Begin
   // Check requested by SREJ blocks
   j := Incr(VS);
   For i := 0 to M - 1 do
     If SBlocks[j].State = _srej then
       Begin
         Slot := j;
         SBlocks[j].State := _wait;
         T := 1;
         Exit;
       End
     Else j := Incr(j);
   Slot := VS;
   SBlocks[VS].State := _wait;
   VS := Incr(VS);
 End;

{==============================================================================}
Function DataToTransmit : boolean;
  Var
    i : byte;
  Begin
    Result := False;
    For i := 0 to M - 1 Do
      If (SBlocks[i].State = _send) or (SBlocks[i].State = _srej) Then
        Begin
         Result := True;
         Exit;
        End;
  End;

{==============================================================================}
Procedure ProcessData(x : UData);
  Var
   i,j : byte;
   zstr : string;
  Begin
   RemoteMachineState := X[0] and $E0;
   j := 0;
   Case (X[0] and $1F) of
     $1F: Begin
            // User block empty. Last status change
          End;
     $1E: Begin
            // User block full. Last status change
            j := 24;
          End;
     $1D: Begin
            // Destructive break signal. User block empty.
          End;
     $1C: Begin
            // Destructive break acknowledge. User block empty.
          End;
     $1B: Begin
            // Used by double status octets PDU.
          End;
   $18 ..$1A: Begin
            // Reserved. Not documented values.
          End
      Else
        Begin
          j := X[0] and $1F;
        End;
   End;
   If j <> 0 Then
    Begin
     zstr := '';
     For i := 1 to j do zstr := zstr + chr(X[i]);
     Write(UserFile1, zstr);
    End;
  End;

{==============================================================================}
Function UEvent( var x: boolean; Change: boolean = True): boolean;
 Begin
  Result := x;
  If x and Change Then x := False;
 End;

{==============================================================================}
Function RLP_CRC(var DBlocks: blocks) : boolean;
 Var
  Polinom: Dword;
  i, CurrentByte, CRC1,CRC2,CRC3 : byte;
 Begin
  Result := False;
  Polinom := $00FFFFFF;
  For i:= 8 to 34 do
   Begin
    CurrentByte := Lo(Polinom) XOR DBlocks[i];
    Polinom     := (Polinom SHR 8) XOR coef[CurrentByte];
   End;
  Polinom := NOT Polinom;
  CRC1 := Lo(Polinom);
  CRC2 := Lo(Polinom SHR 8);
  CRC3 := Lo(Polinom SHR 16);
  If (DBlocks[35] = CRC1) And (DBlocks[36] = CRC2) And (DBlocks[37] = CRC3) Then
   Result := True;
  DBlocks[35] := CRC1;
  DBlocks[36] := CRC2;
  DBlocks[37] := CRC3;
 End;

{==============================================================================}
Procedure IncrementTimers;
  Var
   I : byte;
  Begin
    For i := 0 to M - 1 do If (T_RCVS[i] <> 0) and
                          (T_RCVS[i] <> Timeout1_limit) Then Inc(T_RCVS[i]);
    If (T <> 0) and (T <> Timeout1_limit) Then Inc(T);
    If (T_TEST <> 0) and (T_TEST <> Timeout1_limit) Then Inc(T_TEST);
    If (T_XID <> 0) and (T_XID <> Timeout1_limit) Then Inc(T_XID);
    If (T_RCVR <> 0) and (T_RCVR <> Timeout1_limit) Then Inc(T_RCVR);
  End;

{==============================================================================}
Function CheckTRCVS : byte;
 Var
   i : byte;
 Begin
   i := 0;
   While (i < M) And (T_RCVS[i] <> Timeout1_limit) Do inc(i);
   Result := i;
 End;

{==============================================================================}
Procedure ResetSREJedSlots;
 Var
   i : byte;
 Begin
   For i := 0 to M - 1 do
     If RBlocks[i].State = _srej then RBlocks[i].State := _idle;
 End;

{==============================================================================}
Function SREJedSlots(var x: byte) : boolean;
  Var
    i : byte;
  Begin
    Result := False;
    For i := 0 to M - 1 do
     If RBlocks[i].State = _srej then
       Begin
        x := i;
        Result := True;
        Exit;
       End;
  End;

{==============================================================================}
Procedure ResetAllRnStates;
  Var
    i : byte;
  Begin
    For i := 0 to M - 1 do RBlocks[i].State := _idle;
  End;

{==============================================================================}
Procedure TEST_handling;
 Begin
   TEST_R_State := True;
   TEST_R_FBit := P_F;
   TEST_R_Data := data;
 End;

{==============================================================================}
Function XID_handling : boolean;
 Begin
   Result := False;
    If UEvent(XID_Req) Then
     Begin
      If XID_R_State = _idle Then
        Begin
          Result := True;
          XID_C_State := _send;
          XID_Count := 0;
          XID_C_PBit := True;
          XID_C_Data := inXIDData;
        End
      Else If  XID_R_State = _wait Then
        Begin
          Result := True;
          XID_R_State := _send;
          XID_C_Data := inXIDData;
        End;
     End
    Else If CurrentFrameType = ft_U_XID Then
     Begin
      If Not C_Bit Then
       Begin
        If (XID_C_State = _wait) and P_F Then
          Begin
           Result := True;
           Poll_xchg := False;
           T_XID := 0;
           XID_Ind := True;
           outXIDData := Data;
           XID_C_State := _idle;
          End;
       End
      Else If XID_C_State = _idle then
         Begin
          Result := True;
          XID_Ind := True;
          XID_R_State := _wait;
          XID_R_FBit := P_F;
          outXIDData := Data;
         End
        Else
         Begin
          outXIDData := Data;
          Result := True;
          XID_Ind_Contention := True;
          Poll_xchg := False;
          XID_C_State := _idle;
          T_XID := 1;
         End;
     End
    Else If T_XID  = Timeout1_limit Then
     Begin
      Result := True;
      Poll_xchg := False;
      If XID_Count >= N2 Then NextState := 8
      Else
       Begin
        XID_C_State := _send;
        Inc(XID_Count);
       End;
     End;
 End;

{==============================================================================}
Procedure Init_link_vars;
 Var
  n : integer;
 Begin
  Ackn_State := False;
  Poll_State := _idle;
  Poll_Count := 0;
  Poll_xchg := False;
  SABM_State :=  _idle;
  DISC_State := _idle;
  RRReady := True;
  VA := 0;
  VR := 0;
  VS := 0;
  VD := 0;
//  Init_Slots;
  For n := 0 to M - 1 do
   Begin
    RBlocks[n].State := _idle;
    SBlocks[n].State := _idle;
   End;
 End;

{==============================================================================}
Procedure DeliverAllInSeqIF;
 Begin
  repeat
   Data_Ind := True;
   ProcessData(RBlocks[VR].Data);
   inc(zzzz);
   Form1.Label25.Caption := IntToStr(zzzz);
   RBlocks[VR].State := _idle;
   VR := Incr(VR);
  until RBlocks[VR].State <> _rcvd;
 End;

{==============================================================================}
Procedure MarkMissingIF;
 Var
  n : byte;
 Begin
   n := VR;
   Repeat
    If RBlocks[n].State = _idle Then RBlocks[n].State := _srej;
    n := Incr(n);
   until n = NS;
 End;

{==============================================================================}
Procedure AdvanceVA;
 Begin
  While (VA <> NR) do
   Begin
    SBlocks[VA].State := _idle;
    VA := Incr(VA);
   End;
 End;

{==============================================================================}
Procedure DecreaseVS;
 Begin
  While (VS <> NR) do
   Begin
    VS := Decr(VS);
    SBlocks[VA].State := _idle;
   End;
 End;

{==============================================================================}
Function Send_TXU: boolean;
 Begin
  Result := False;
  If UEvent(TEST_R_State) then
   Begin
    SendFrame(ft_U_TEST,False,TEST_R_FBit,0,0,TEST_R_Data);
    result := True;
   End
  Else If XID_R_State = _send Then
   Begin
    SendFrame(ft_U_XID,False,True,0,0,XID_C_Data);
    XID_R_State := _idle;
    result := True;
   End
  Else If (XID_C_State = _send) and (Not Poll_xchg ) Then
   Begin
    SendFrame(ft_U_XID,True,True,0,0,XID_C_Data);
    XID_C_State := _wait;
    T_XID := 1;
    Poll_xchg := True;
    result := True;
   End
  Else If UEvent(UI_State) Then
   Begin
    SendFrame(ft_U_UI,True,False,0,0,UI_Data);
    result := True;
   End;
 End;

{==============================================================================}
Procedure SendData;
 Var
   n : byte;
 Begin
  If UEvent(UA_State) Then SendFrame(ft_U_UA,False,UA_FBit,0,0,ZeroData)
  Else If Ackn_FBit Then
     If LRReady Then Send_XX_or_XX_I__response(ft_S_RR)
                Else Send_XX_or_XX_I__response(ft_S_RNR)
  Else If SREJedSlots(n) Then SendSREJ_command(n)
  Else If LRReady Then Send_XX_or_XX_I__command(ft_S_RR)
                  Else Send_XX_or_XX_I__command(ft_S_RNR);
 End;

{==============================================================================}
Procedure Send_XX_or_XX_I__response(X: byte);
 Var
   k : byte;
 Begin
   If (Not DataToTransmit) or
      (Not WindowOpen) or
      (Not RRReady) Then
        Begin
          SendFrame(X,False,True,VR,0,ZeroData);
          ACKN_State := False;
          ACKN_FBit := False;
        End
   Else
     Begin
       PrepareDataToTransmit(k);
       SendFrame(X + 4,False,True,VR,k,SBlocks[k].Data);
       ACKN_State := False;
       ACKN_FBit := False;
     End;
 End;

{==============================================================================}
Procedure SendSREJ_command(X:byte);
 Var
  k : byte;
 Begin
   Inc(xx2);
   Form1.Label29.Caption := IntToStr(xx2);
   If (Not Poll_xchg) and (Poll_State = _send) Then
     Begin
       SendFrame(ft_S_SREJ,True,True,x,0,ZeroData);
       RBlocks[x].State := _wait;
       T_RCVS[x] := 1;
       inc(Poll_Count);
       Poll_State := _wait;
       Poll_xchg := True;
       T := 1;
     End
   Else If DataToTransmit and WindowOpen and RRReady Then
     Begin
       PrepareDataToTransmit(k);
       SendFrame(ft_SplusI_SREJ,True,False,x,k,SBlocks[k].Data);
       RBlocks[x].State := _wait;
       T_RCVS[x] := 1;
     End
   Else
     Begin
       SendFrame(ft_S_SREJ,True,False,x,0,ZeroData);
       RBlocks[x].State := _wait;
       T_RCVS[x] := 1;
     End;
 End;

{==============================================================================}
Procedure Send_XX_or_XX_I__command(X: byte);
 Var
  k : byte;
 Begin
   If (Not Poll_xchg) and (Poll_State = _send) Then
     Begin
       SendFrame(X,True,True,VR,0,ZeroData);
       ACKN_State := False;
       inc(Poll_Count);
       Poll_State := _wait;
       Poll_xchg := True;
       T := 1;
     End
   Else If DataToTransmit and WindowOpen and RRReady Then
     Begin
       PrepareDataToTransmit(k);
       SendFrame(X + 4,True,False,VR,k,SBlocks[k].Data);
       ACKN_State := False;
     End
   Else If (Not ACKN_State) and (DTX_SF = X)
            and (DTX_VR = VR) Then
             SendFrame(X,True,False,VR,0,ZeroData,True)
   Else
     Begin
       SendFrame(X,True,False,VR,0,ZeroData);
       ACKN_State := False;
       DTX_SF := X;
       DTX_VR := VR;
     End;
 End;

{==============================================================================}
Procedure ResetAllT_RCVS;
 Var
   i: byte;
 Begin
   For i := 0 to M - 1 do T_RCVS[i] := 0;
 End;

{==============================================================================}
Function Check_input_PDU (var X: Blocks; var Y: Blocks): boolean;
 Var
  RLPHeader : word;
  i : byte;
 Begin
   IncrementTimers;

   Result := True;
   RLPHeader := X[8] * 256 + X[9];

   CurrentFrameType := ft_bad;
   For i := 0 to 24 do Data[i] := X[10 + i];
   P_F := (RLPHeader And $0002) = $0002;
   C_Bit := (RLPHeader And $0100) = $0100;
   NR := (RLPHeader And $00FC) shr 2;
   NS := ((RLPHeader And $F800) shr 11) + ((RLPHeader And $0001) shl 5);

   If Not RLP_CRC(X) Then
     Begin
       MainStateMachine;
       Y := outBlock;
       Exit;
     End;

   Case (RLPHeader And $F801) of
     $F801 : Case (Lo(RLPHeader) And $7C) of
               $1C : If ((RLPHeader And $0102) = $0102) Then
                             CurrentFrameType := ft_U_SABM;
               $30 : If ((RLPHeader And $0100) = $0000) Then
                             CurrentFrameType := ft_U_UA;
               $20 : If ((RLPHeader And $0100) = $0100) Then
                             CurrentFrameType := ft_U_DISC;
               $0C : If ((RLPHeader And $0100) = $0000) Then
                             CurrentFrameType := ft_U_DM;
               $3C : Begin
                             CurrentFrameType := ft_U_NULL;
                     End;
               $00 : CurrentFrameType := ft_U_UI;
               $5C : CurrentFrameType := ft_U_XID;
               $70 : If ((RLPHeader And $0100) = $0100) Then
                             CurrentFrameType := ft_U_TEST;
             End;
     $F001 : Case (Hi(RLPHeader) And $06) of
               $00 : CurrentFrameType := ft_S_RR;
               $04 : CurrentFrameType := ft_S_REJ;
               $02 : CurrentFrameType := ft_S_RNR;
               $06 : CurrentFrameType := ft_S_SREJ;
             End
     Else Case (Hi(RLPHeader) And $06) of
            $00 : CurrentFrameType := ft_SplusI_RR;
            $04 : CurrentFrameType := ft_SplusI_REJ;
            $02 : CurrentFrameType := ft_SplusI_RNR;
            $06 : CurrentFrameType := ft_SplusI_SREJ;
          End;
   End;
   MainStateMachine;
   Y := outBlock;
 End;

{==============================================================================}
Function I_Handler:boolean;
 Begin
   Result := False;
   If (C_Bit and P_F) or (NS >= M) Then
    Begin
     Result := True;
     Exit;
    End;
   If NS = VR Then
     Begin
       RBlocks[VR].State := _rcvd;
       RBlocks[VR].Data := Data;
       DeliverAllInSeqIF;
       ACKN_State := True;
     End
   Else //  SREJ Implemented for error recovery
    Begin
      If RBlocks[NS].State = _wait Then T_RCVS[NS] := 0;
      RBlocks[NS].State := _rcvd;
      RBlocks[NS].Data  := data;
      MarkMissingIF;
    End;
 End;

{==============================================================================}
Function S_Handler:boolean;
 Begin
  Result := False;
  If C_Bit and P_F Then
    Begin
        Inc(xx3);
        Form1.Label30.Caption := IntToStr(xx3);
      ACKN_State := True;
      ACKN_FBit := True;
      ResetAllRnStates;
      ResetAllT_RCVS;
      T_RCVR := 0;
    End;
  If Poll_state <> _idle Then
   Begin
     If Not P_F Then Exit;
     If (SF = ft_S_SREJ) or (SF = ft_S_REJ) or
        (SF = ft_SplusI_SREJ) or (SF = ft_SplusI_REJ) Then Exit;
     DecreaseVS;
     Poll_State := _idle;
     Poll_xchg := False;
   End;
  Case SF of
    ft_S_RR, ft_SplusI_RR : Begin
                              AdvanceVA;
                              RRReady := True;
                            End;
  ft_S_RNR, ft_SplusI_RNR : Begin
                              AdvanceVA;
                              RRReady := False;
                            End;
  ft_S_REJ, ft_SplusI_REJ : Begin
                              AdvanceVA;
                              RRReady := True;
                              DecreaseVS;
                            End;
   ft_S_SREJ, ft_SplusI_SREJ : Begin
                                inc(zz3);
                                Form1.Label10.Caption := IntToStr(zz3);
                                SBlocks[NR].State := _srej;
                                T := 0;
                                Exit;
                               End;
  End;
  If VA <> VS Then Exit;
  T := 0;
 End;

{==============================================================================}
Procedure SendFrame(outFT : byte; outCR, outPF: boolean;
           outNR, outNS : byte; outData: UData; outDTX: boolean = False);
 Var
   outHeader : word;
   outNR1, outNS1: word;
   i : word;
 Begin
   outNR1 := outNR shl 2;
   outNS1 := (outNS shl 11) + (outNS shr 5);
   Case outFT of
     ft_U_SABM : outHeader := $F91F;
     ft_U_UA   : If outPF Then outHeader := $F833 Else outHeader := $F831;
     ft_U_DISC : If outPF Then outHeader := $F923 Else outHeader := $F921;
     ft_U_DM   : If outPF Then outHeader := $F80F Else outHeader := $F80D;
     ft_U_NULL : outHeader := $F83D;
     ft_U_UI : Begin
                 If outPF Then outHeader := $F803 Else outHeader := $F801;
                 If outCR Then outHeader := outHeader or $0100;
               End;
     ft_U_XID : If outCR Then outHeader := $F95F Else outHeader := $F85F;
     ft_U_TEST : Begin
                   If outPF Then outHeader := $F873 Else outHeader := $F871;
                   If outCR Then outHeader := outHeader or $0100;
                 End;
     ft_S_RR : Begin
                 If outPF Then outHeader := $F003 Else outHeader := $F001;
                 If outCR Then outHeader := outHeader or $0100;
                 outHeader := outHeader or outNR1;
               End;
     ft_S_REJ : Begin
                 If outPF Then outHeader := $F403 Else outHeader := $F401;
                 If outCR Then outHeader := outHeader or $0100;
                 outHeader := outHeader or outNR1;
                End;
     ft_S_RNR : Begin
                 If outPF Then outHeader := $F203 Else outHeader := $F201;
                 If outCR Then outHeader := outHeader or $0100;
                 outHeader := outHeader or outNR1;
                End;
     ft_S_SREJ : Begin
                 If outPF Then outHeader := $F603 Else outHeader := $F601;
                 If outCR Then outHeader := outHeader or $0100;
                 outHeader := outHeader or outNR1;
                 End;
     ft_SplusI_RR : Begin
                      outHeader := $0000 or outNS1 or outNR1;
                      If outPF Then outHeader := outHeader or $0002;
                      If outCR Then outHeader := outHeader or $0100;
                    End;
     ft_SplusI_REJ : Begin
                       outHeader := $0400 or outNS1 or outNR1;
                       If outPF Then outHeader := outHeader or $0002;
                       If outCR Then outHeader := outHeader or $0100;
                     End;
     ft_SplusI_RNR : Begin
                       outHeader := $0200 or outNS1 or outNR1;
                       If outPF Then outHeader := outHeader or $0002;
                       If outCR Then outHeader := outHeader or $0100;
                     End;
     ft_SplusI_SREJ : Begin
                        outHeader := $0600 or outNS1 or outNR1;
                        If outPF Then outHeader := outHeader or $0002;
                        If outCR Then outHeader := outHeader or $0100;
                      End
             else
                outHeader := $0000;
   End;
   outBlock[0] := $1E; outBlock[1] := $00; outBlock[2] := $0C;
   outBlock[3] := $F0; outBlock[4] := $00; outBlock[5] := $20;
   If outDTX Then outBlock[6] := $01 else outBlock[6] := $00;
   outBlock[7] := $D9;
   outBlock[8] := Hi(outHeader);
   outBlock[9] := Lo(outHeader);
   For i:= 0 to 24 do outBlock[10 + i] := outData[i];
   If outFT <> ft_bad Then RLP_CRC(outBlock);
 End;

{==============================================================================}
Procedure MainStateMachine;
 Var
   i : byte;
 Begin
  Case CurrentState of

  {****************** S T A T E   0  **************************}
     0: Begin
          Case CurrentFrameType of
            ft_U_DISC : SendFrame(ft_U_DM,False,P_F,0,0,ZeroData);
            ft_U_SABM : SendFrame(ft_U_DM,False,True,0,0,ZeroData)
            Else
              Begin
                SendFrame(ft_U_NULL,False,False,0,0,ZeroData);
                If UEvent(Attach_Req) Then
                  Begin
                    NextState := 1;
                    UA_State := False;
                  End;
              End;
          End;
        End;
  {****************** S T A T E   1  **************************}
     1: Begin
         If Not XID_handling Then
          Case CurrentFrameType of
            ft_U_TEST : TEST_handling;
            ft_BAD: Begin end;
            ft_U_SABM : Begin
                          Conn_Ind := True;
                          NextState := 3;
                        End;
            ft_U_DISC : Begin
                          UA_State := True;
                          UA_FBit := P_F;
                        End
            Else If UEvent(Conn_Req) Then
                 Begin
                   SABM_State := _send;
                   SABM_Count := 0;
                   NextState := 2;
                 End;
          End;
          If Not Send_TXU Then
           Begin
            If UEvent(UA_State) Then SendFrame(ft_U_UA,False,UA_FBit,0,0,ZeroData)
             Else SendFrame(ft_U_NULL,False,False,0,0,ZeroData);
            End;
        End;

  {****************** S T A T E   2  **************************}
     2: Begin
         If Not XID_handling Then
          Case CurrentFrameType of
            ft_U_TEST : TEST_handling;
            ft_U_SABM : Begin
                          T := 0;
                          Conn_Conf := True;
                          UA_State := True;
                          UA_FBit := True;
                          Init_Link_Vars;
                          NextState := 4;
                        End;
            ft_U_DISC : Begin
                          T := 0;
                          DISC_Ind := True;
                          UA_State := True;
                          UA_FBit := P_F;
                          NextState := 1;
                        End;
              ft_U_UA : If (SABM_State = _wait) and P_F Then
                              Begin
                                T := 0;
                                Conn_Conf := True;
                                Init_Link_Vars;
                                NextState := 4;
                              End;
              ft_U_DM : If (SABM_State = _wait) and P_F Then
                              Begin
                                T := 0;
                                Poll_xchg := False;
                                Conn_Conf_Neg := True;
                                NextState := 1;
                              End
              Else If T = Timeout1_limit then
                    Begin
                      Poll_xchg := False;
                      If SABM_Count > N2 then NextState := 8;
                      SABM_State := _send;
                    End;
          End;
          If (Not Send_TXU) Then
            If (SABM_State = _send) and (Poll_xchg = False) Then
              Begin
                SendFrame(ft_U_SABM,True,True,0,0,ZeroData);
                SABM_State := _wait;
                inc(SABM_Count);
                Poll_xchg := True;
                T := 1;
              End
            Else SendFrame(ft_U_NULL,False,False,0,0,ZeroData);
         End;


  {****************** S T A T E   3  **************************}
     3: Begin
         If Not XID_handling Then
          Case CurrentFrameType of
            ft_U_TEST : TEST_handling;
            ft_U_DISC : Begin
                          DISC_Ind := True;
                          UA_State := True;
                          UA_FBit := P_F;
                          NextState := 1;
                        End
            Else If Conn_Resp_Neg Then
                  Begin
                    DM_State := True;
                    DM_Fbit := True;
                    NextState := 1;
                  End
                Else If Conn_Resp Then
                  Begin
                    UA_State := True;
                    UA_FBit := True;
                    Init_Link_Vars;
                    NextState := 4;
                  End;
          End;
          If (Not Send_TXU) Then SendFrame(ft_U_NULL,False,False,0,0,ZeroData);
        End;


  {****************** S T A T E   4  **************************}
     4: Begin
         If (Not Send_TXU) Then SendData;
         If Not XID_handling Then
          Case CurrentFrameType of
            ft_U_TEST : TEST_handling;
            ft_U_DISC : Begin
                          T := 0;
                          T_RCVR := 0;
                          ResetAllT_RCVS;
                          Disc_Ind := True;
                          UA_State := True;
                          UA_FBit := P_F;
                          NextState := 1;
                        End;
            ft_U_SABM : Begin
                          T := 0;
                          T_RCVR := 0;
                          ResetAllT_RCVS;
                          Reset_Ind := True;
                          NextState := 7;
                        End;
              ft_S_RR,
             ft_S_RNR,
             ft_S_REJ,
            ft_S_SREJ : Begin
                          SF := CurrentFrameType;
                          If {(Not P_F) and }(NR < M) Then S_Handler;
                        End;
         ft_SplusI_RR,
        ft_SplusI_RNR,
        ft_SplusI_REJ,
       ft_SplusI_SREJ : Begin
                          SF := CurrentFrameType;
                          If (Not P_F) and (NR < M) Then
                            If Not (I_Handler) Then S_Handler;
                        End;
          End;
          If SBlocks[VD].State = _idle Then
                      Begin
                        If UEvent(Data_Req) Then
                          Begin
                           SBlocks[VD].Data := UserData;
                           SBlocks[VD].State := _send;
                           VD := Incr(VD);
                          End;
                      End
                    Else If UEvent(Reset_Req) Then
                      Begin
                        T := 0;
                        T_RCVR := 0;
                        ResetAllT_RCVS;
                        SABM_State := _send;
                        SABM_Count := 0;
                        NextState := 6;
                      End
                    Else If T_RCVR = Timeout1_limit Then
                      Begin
                        ResetSREJedSlots;
                      End
                    Else If T = Timeout1_limit Then
                      Begin
                        Poll_xchg := False;
                        If Poll_State = _idle Then
                          Begin
                            inc(zz1);
                            Form1.Label9.Caption := IntToStr(zz1);
                            Poll_State := _send;
                            Poll_Count := 0;
                          End
                        Else If Poll_State = _wait Then
                          Begin
                            If Poll_Count > N2 Then NextState := 8;
                            Poll_State := _send;
                            inc(Poll_Count);
                          End;
                      End
                     Else
                       Begin
                         i := CheckTRCVS;
                         If i <> M Then RBlocks[i].State := _srej;
                       End;
        End;
  {****************** S T A T E   5  **************************}
     5: Begin
          // Not implemented. Because DISC_Req not supported;
        End;

  {****************** S T A T E   6  **************************}
     6: Begin
         If Not XID_handling Then
          Case CurrentFrameType of
            ft_U_TEST : TEST_handling;
            ft_U_DISC : Begin
                          T := 0;
                          DISC_Ind := True;
                          UA_State := True;
                          UA_FBit := P_F;
                          NextState := 1;
                        End;
            ft_U_SABM : Begin
                          T := 0;
                          Reset_Conf := True;
                          UA_State := True;
                          UA_FBit := True;
                          Init_Link_Vars;
                          NextState := 4;
                        End;
              ft_U_UA : If (SABM_State = _wait) and P_F Then
                            Begin
                              T := 0;
                              Reset_Conf := True;
                              Init_Link_Vars;
                              NextState := 4;
                            End
                  Else If T = Timeout1_limit Then
                        Begin
                          Poll_xchg := False;
                          If SABM_Count > N2 Then NextState := 8;
                          SABM_State := _send;
                        End;
          End;
          If (Not Send_TXU) Then
              If (SABM_State = _wait) or Poll_xchg Then
                  SendFrame(ft_U_NULL,False,False,0,0,ZeroData)
              Else
                Begin
                  SendFrame(ft_U_SABM,True,True,0,0,ZeroData);
                  SABM_State := _wait;
                  inc(SABM_Count);
                  Poll_xchg := True;
                  T := 1;
                End;
        End;

  {****************** S T A T E   7  **************************}
     7: Begin
         If Not XID_handling Then
          Case CurrentFrameType of
            ft_U_TEST : TEST_handling;
            ft_U_DISC : Begin
                          DISC_Ind := True;
                          UA_State := True;
                          UA_FBit := P_F;
                          NextState := 1;
                        End
                  Else If UEvent(Reset_Resp) Then
                    Begin
                      UA_State := True;
                      UA_FBit := True;
                      Init_Link_Vars;
                      NextState :=4;
                    End;
          End;
          If (Not Send_TXU) Then SendFrame(ft_U_NULL,False,False,0,0,ZeroData);
        End;

  {****************** S T A T E   8  **************************}
     8: Begin
          // Undefined by current standard
          Error_Ind := True;
        End;
  End;
  CurrentState := NextState;
 End;

{==============================================================================}
Procedure RLPInit;
 Begin
  CurrentState := 0;
  WindowOpen := True;
  LRReady := True;
  Attach_Req := False;
  Conn_Ind := False;
  Conn_Conf := False;
  Conn_Conf_Neg := False;
  Conn_Req := False;
  Conn_Resp := False;
  Conn_Resp_Neg := False;
  Data_Ind := False;
  Data_Req := False;
  DISC_Ind := False;
  Error_Ind := False;
  Reset_Conf := False;
  Reset_Ind := False;
  Reset_Req := False;
  Reset_Resp := False;
  XID_Req := False;
  XID_Ind_Contention := False;
  XID_Ind := False;
  Ackn_FBit := False;
  DM_FBit := False;
  DM_State:= False;
  DTX_SF := 0;
  DTX_VR := 0;
  SABM_Count := 0;
  SF :=0;
  T :=0;
  TEST_R_FBit := False;
  TEST_R_State := False;
  T_RCVR := 0;
  T_TEST := 0;
  T_XID := 0;
  UA_FBit := False;
  UA_State := False;
  UI_State := False;
  XID_Count := 0;
  XID_C_PBit := False;
  XID_C_State := _idle;
  XID_R_FBit := False;
  XID_R_State := _idle;
  ResetAllT_RCVS;
  Init_link_vars;
  zzzz := 0;
  zz1 := 0;
  zz2 := 0;
  zz3 := 0;
  xx1 := 0;
  xx2 := 0;
  xx3 := 0;
 End;

Begin
  RLPInit;
End.  */
