/**
  ******************************************************************************
  * @file    usb_regs.h
  * @date    05-December-2013
  * @brief   hardware registers
  ******************************************************************************
  */

#ifndef usb_otg_regs_h
#define usb_otg_regs_h

#define USB_OTG_FS_BASE_ADDR                 0x50000000

#define USB_OTG_CORE_GLOBAL_REGS_OFFSET      0x000
#define USB_OTG_DEV_GLOBAL_REG_OFFSET        0x800
#define USB_OTG_DEV_IN_EP_REG_OFFSET         0x900
#define USB_OTG_EP_REG_OFFSET                0x20
#define USB_OTG_DEV_OUT_EP_REG_OFFSET        0xB00
#define USB_OTG_HOST_GLOBAL_REG_OFFSET       0x400
#define USB_OTG_HOST_PORT_REGS_OFFSET        0x440
#define USB_OTG_HOST_CHAN_REGS_OFFSET        0x500
#define USB_OTG_CHAN_REGS_OFFSET             0x20
#define USB_OTG_PCGCCTL_OFFSET               0xE00
#define USB_OTG_DATA_FIFO_OFFSET             0x1000
#define USB_OTG_DATA_FIFO_SIZE               0x1000

#define USB_OTG_MAX_TX_FIFOS                 15

#define USB_OTG_HS_MAX_PACKET_SIZE           512
#define USB_OTG_FS_MAX_PACKET_SIZE           64
#define USB_OTG_MAX_EP0_SIZE                 64

typedef struct _USB_OTG_GREGS { //000h
  volatile unsigned int GOTGCTL;      /* USB_OTG Control and Status Register    000h*/
  volatile unsigned int GOTGINT;      /* USB_OTG Interrupt Register             004h*/
  volatile unsigned int GAHBCFG;      /* Core AHB Configuration Register    008h*/
  volatile unsigned int GUSBCFG;      /* Core USB Configuration Register    00Ch*/
  volatile unsigned int GRSTCTL;      /* Core Reset Register                010h*/
  volatile unsigned int GINTSTS;      /* Core Interrupt Register            014h*/
  volatile unsigned int GINTMSK;      /* Core Interrupt Mask Register       018h*/
  volatile unsigned int GRXSTSR;      /* Receive Sts Q Read Register        01Ch*/
  volatile unsigned int GRXSTSP;      /* Receive Sts Q Read & POP Register  020h*/
  volatile unsigned int GRXFSIZ;      /* Receive FIFO Size Register         024h*/
  volatile unsigned int DIEPTXF0_HNPTXFSIZ;   /* EP0 / Non Periodic Tx FIFO Size Register 028h*/
  volatile unsigned int HNPTXSTS;     /* Non Periodic Tx FIFO/Queue Sts reg 02Ch*/
  volatile unsigned int GI2CCTL;      /* I2C Access Register                030h*/
  unsigned int Reserved34;  /* PHY Vendor Control Register        034h*/
  volatile unsigned int GCCFG;        /* General Purpose IO Register        038h*/
  volatile unsigned int CID;          /* User ID Register                   03Ch*/
  unsigned int  Reserved40[48];   /* Reserved                      040h-0FFh*/
  volatile unsigned int HPTXFSIZ; /* Host Periodic Tx FIFO Size Reg     100h*/
  volatile unsigned int DIEPTXF[USB_OTG_MAX_TX_FIFOS];/* dev Periodic Transmit FIFO */
} USB_OTG_GREGS;

typedef struct _USB_OTG_DREGS { // 800h
  volatile unsigned int DCFG;         /* dev Configuration Register   800h*/
  volatile unsigned int DCTL;         /* dev Control Register         804h*/
  volatile unsigned int DSTS;         /* dev Status Register (RO)     808h*/
  unsigned int Reserved0C;            /* Reserved                     80Ch*/
  volatile unsigned int DIEPMSK;      /* dev IN Endpoint Mask         810h*/
  volatile unsigned int DOEPMSK;      /* dev OUT Endpoint Mask        814h*/
  volatile unsigned int DAINT;        /* dev All Endpoints Itr Reg    818h*/
  volatile unsigned int DAINTMSK;     /* dev All Endpoints Itr Mask   81Ch*/
  unsigned int  Reserved20;           /* Reserved                     820h*/
  unsigned int Reserved9;             /* Reserved                     824h*/
  volatile unsigned int DVBUSDIS;     /* dev VBUS discharge Register  828h*/
  volatile unsigned int DVBUSPULSE;   /* dev VBUS Pulse Register      82Ch*/
  volatile unsigned int DTHRCTL;      /* dev thr                      830h*/
  volatile unsigned int DIEPEMPMSK;   /* dev empty msk                834h*/
  volatile unsigned int DEACHINT;     /* dedicated EP interrupt       838h*/
  volatile unsigned int DEACHMSK;     /* dedicated EP msk             83Ch*/
  unsigned int Reserved40;            /* dedicated EP mask            840h*/
  volatile unsigned int DINEP1MSK;    /* dedicated EP mask            844h*/
  unsigned int  Reserved44[15];       /* Reserved                     844-87Ch*/
  volatile unsigned int DOUTEP1MSK;   /* dedicated EP msk             884h*/
} USB_OTG_DREGS;

/** volatile constN_Endpoint-Specific_Register
  */
typedef struct _USB_OTG_INEPREGS {
  volatile unsigned int DIEPCTL; /* dev IN Endpoint Control Reg 900h + (ep_num * 20h) + 00h*/
  unsigned int Reserved04;       /* Reserved                       900h + (ep_num * 20h) + 04h*/
  volatile unsigned int DIEPINT; /* dev IN Endpoint Itr Reg     900h + (ep_num * 20h) + 08h*/
  unsigned int Reserved0C;       /* Reserved                       900h + (ep_num * 20h) + 0Ch*/
  volatile unsigned int DIEPTSIZ;/* IN Endpoint Txfer Size   900h + (ep_num * 20h) + 10h*/
  volatile unsigned int DIEPDMA; /* IN Endpoint DMA Address Reg    900h + (ep_num * 20h) + 14h*/
  volatile unsigned int DTXFSTS; /* IN Endpoint Tx FIFO Status Reg 900h + (ep_num * 20h) + 18h*/
  unsigned int Reserved18;       /* Reserved  900h+(ep_num*20h)+1Ch-900h+ (ep_num * 20h) + 1Ch*/
} USB_OTG_INEPREGS;

typedef struct _USB_OTG_OUTEPREGS {
  volatile unsigned int DOEPCTL;     /* dev OUT Endpoint Control Reg  B00h + (ep_num * 20h) + 00h*/
  volatile unsigned int DOUTEPFRM;   /* dev OUT Endpoint Frame number B00h + (ep_num * 20h) + 04h*/
  volatile unsigned int DOEPINT;     /* dev OUT Endpoint Itr Reg      B00h + (ep_num * 20h) + 08h*/
  unsigned int Reserved0C;           /* Reserved                         B00h + (ep_num * 20h) + 0Ch*/
  volatile unsigned int DOEPTSIZ;    /* dev OUT Endpoint Txfer Size   B00h + (ep_num * 20h) + 10h*/
  volatile unsigned int DOEPDMA;     /* dev OUT Endpoint DMA Address  B00h + (ep_num * 20h) + 14h*/
  unsigned int Reserved18[2];        /* Reserved B00h + (ep_num * 20h) + 18h - B00h + (ep_num * 20h) + 1Ch*/
} USB_OTG_OUTEPREGS;

typedef struct _USB_OTG_HREGS {
  volatile unsigned int HCFG;      /* Host Configuration Register    400h*/
  volatile unsigned int HFIR;      /* Host Frame Interval Register   404h*/
  volatile unsigned int HFNUM;     /* Host Frame Nbr/Frame Remaining 408h*/
  unsigned int Reserved40C;        /* Reserved                       40Ch*/
  volatile unsigned int HPTXSTS;   /* Host Periodic Tx FIFO/ Queue Status 410h*/
  volatile unsigned int HAINT;     /* Host All Channels Interrupt Register 414h*/
  volatile unsigned int HAINTMSK;  /* Host All Channels Interrupt Mask 418h*/
} USB_OTG_HREGS;

typedef struct _USB_OTG_HC_REGS {
  volatile unsigned int HCCHAR;
  volatile unsigned int HCSPLT;
  volatile unsigned int HCINT;
  volatile unsigned int HCGINTMSK;
  volatile unsigned int HCTSIZ;
  volatile unsigned int HCDMA;
  unsigned int Reserved[2];
} USB_OTG_HC_REGS;

typedef struct USB_OTG_core_regs { //000h
  USB_OTG_GREGS         *GREGS;
  USB_OTG_DREGS         *DREGS;
  USB_OTG_HREGS         *HREGS;
  USB_OTG_INEPREGS      *INEP_REGS[USB_OTG_MAX_TX_FIFOS];
  USB_OTG_OUTEPREGS     *OUTEP_REGS[USB_OTG_MAX_TX_FIFOS];
  USB_OTG_HC_REGS       *HC_REGS[USB_OTG_MAX_TX_FIFOS];
  volatile unsigned int *HPRT0;
  volatile unsigned int *DFIFO[USB_OTG_MAX_TX_FIFOS];
  volatile unsigned int *PCGCCTL;
} USB_OTG_CORE_REGS, *PUSB_OTG_CORE_REGS;

typedef union _USB_OTG_OTGCTL_TypeDef {
  unsigned int d32;
  struct {
    unsigned int sesreqscs : 1;
    unsigned int sesreq : 1;
    unsigned int Reserved2_7 : 6;
    unsigned int hstnegscs : 1;
    unsigned int hnpreq : 1;
    unsigned int hstsethnpen : 1;
    unsigned int devhnpen : 1;
    unsigned int Reserved12_15 : 4;
    unsigned int conidsts : 1;
    unsigned int Reserved17 : 1;
    unsigned int asesvld : 1;
    unsigned int bsesvld : 1;
    unsigned int currmod : 1;
    unsigned int Reserved21_31 : 11;
  } b;
} USB_OTG_OTGCTL_TypeDef ;
typedef union _USB_OTG_GOTGINT_TypeDef {
  unsigned int d32;
  struct {
    unsigned int Reserved0_1 : 2;
    unsigned int sesenddet : 1;
    unsigned int Reserved3_7 : 5;
    unsigned int sesreqsucstschng : 1;
    unsigned int hstnegsucstschng : 1;
    unsigned int reserver10_16 : 7;
    unsigned int hstnegdet : 1;
    unsigned int adevtoutchng : 1;
    unsigned int debdone : 1;
    unsigned int Reserved31_20 : 12;
  } b;
} USB_OTG_GOTGINT_TypeDef ;
typedef union _USB_OTG_GAHBCFG_TypeDef {
  unsigned int d32;
  struct {
    unsigned int glblintrmsk : 1;
    unsigned int hburstlen : 4;
    unsigned int dmaenable : 1;
    unsigned int Reserved : 1;
    unsigned int nptxfemplvl_txfemplvl : 1;
    unsigned int ptxfemplvl : 1;
    unsigned int Reserved9_31 : 23;
  } b;
} USB_OTG_GAHBCFG_TypeDef ;
typedef union _USB_OTG_GUSBCFG_TypeDef {
  unsigned int d32;
  struct {
    unsigned int toutcal : 3;
    unsigned int phyif : 1;
    unsigned int ulpi_utmi_sel : 1;
    unsigned int fsintf : 1;
    unsigned int physel : 1;
    unsigned int ddrsel : 1;
    unsigned int srpcap : 1;
    unsigned int hnpcap : 1;
    unsigned int usbtrdtim : 4;
    unsigned int nptxfrwnden : 1;
    unsigned int phylpwrclksel : 1;
    unsigned int otgutmifssel : 1;
    unsigned int ulpi_fsls : 1;
    unsigned int ulpi_auto_res : 1;
    unsigned int ulpi_clk_sus_m : 1;
    unsigned int ulpi_ext_vbus_drv : 1;
    unsigned int ulpi_int_vbus_indicator : 1;
    unsigned int term_sel_dl_pulse : 1;
    unsigned int Reserved : 6;
    unsigned int force_host : 1;
    unsigned int force_dev : 1;
    unsigned int corrupt_tx : 1;
  } b;
} USB_OTG_GUSBCFG_TypeDef ;
typedef union _USB_OTG_GRSTCTL_TypeDef {
  unsigned int d32;
  struct {
    unsigned int csftrst : 1;
    unsigned int hsftrst : 1;
    unsigned int hstfrm : 1;
    unsigned int intknqflsh : 1;
    unsigned int rxfflsh : 1;
    unsigned int txfflsh : 1;
    unsigned int txfnum : 5;
    unsigned int Reserved11_29 : 19;
    unsigned int dmareq : 1;
    unsigned int ahbidle : 1;
  } b;
} USB_OTG_GRSTCTL_TypeDef ;
typedef union _USB_OTG_GINTMSK_TypeDef {
  unsigned int d32;
  struct {
    unsigned int Reserved0 : 1;
    unsigned int modemismatch : 1;
    unsigned int otgintr : 1;
    unsigned int sofintr : 1;
    unsigned int rxstsqlvl : 1;
    unsigned int nptxfempty : 1;
    unsigned int ginnakeff : 1;
    unsigned int goutnakeff : 1;
    unsigned int Reserved8 : 1;
    unsigned int i2cintr : 1;
    unsigned int erlysuspend : 1;
    unsigned int usbsuspend : 1;
    unsigned int usbreset : 1;
    unsigned int enumdone : 1;
    unsigned int isooutdrop : 1;
    unsigned int eopframe : 1;
    unsigned int Reserved16 : 1;
    unsigned int epmismatch : 1;
    unsigned int inepintr : 1;
    unsigned int outepintr : 1;
    unsigned int incomplisoin : 1;
    unsigned int incomplisoout : 1;
    unsigned int Reserved22_23 : 2;
    unsigned int portintr : 1;
    unsigned int hcintr : 1;
    unsigned int ptxfempty : 1;
    unsigned int Reserved27 : 1;
    unsigned int conidstschng : 1;
    unsigned int disconnect : 1;
    unsigned int sessreqintr : 1;
    unsigned int wkupintr : 1;
  } b;
} USB_OTG_GINTMSK_TypeDef ;
typedef union _USB_OTG_GINTSTS_TypeDef {
  unsigned int d32;
  struct {
    unsigned int curmode : 1;
    unsigned int modemismatch : 1;
    unsigned int otgintr : 1;
    unsigned int sofintr : 1;
    unsigned int rxstsqlvl : 1;
    unsigned int nptxfempty : 1;
    unsigned int ginnakeff : 1;
    unsigned int goutnakeff : 1;
    unsigned int Reserved8 : 1;
    unsigned int i2cintr : 1;
    unsigned int erlysuspend : 1;
    unsigned int usbsuspend : 1;
    unsigned int usbreset : 1;
    unsigned int enumdone : 1;
    unsigned int isooutdrop : 1;
    unsigned int eopframe : 1;
    unsigned int intimerrx : 1;
    unsigned int epmismatch : 1;
    unsigned int inepint: 1;
    unsigned int outepintr : 1;
    unsigned int incomplisoin : 1;
    unsigned int incomplisoout : 1;
    unsigned int Reserved22_23 : 2;
    unsigned int portintr : 1;
    unsigned int hcintr : 1;
    unsigned int ptxfempty : 1;
    unsigned int Reserved27 : 1;
    unsigned int conidstschng : 1;
    unsigned int disconnect : 1;
    unsigned int sessreqintr : 1;
    unsigned int wkupintr : 1;
  } b;
} USB_OTG_GINTSTS_TypeDef ;
typedef union _USB_OTG_DRXSTS_TypeDef {
  unsigned int d32;
  struct {
    unsigned int epnum : 4;
    unsigned int bcnt : 11;
    unsigned int dpid : 2;
    unsigned int pktsts : 4;
    unsigned int fn : 4;
    unsigned int Reserved : 7;
  } b;
} USB_OTG_DRXSTS_TypeDef ;
typedef union _USB_OTG_GRXSTS_TypeDef {
  unsigned int d32;
  struct {
    unsigned int chnum : 4;
    unsigned int bcnt : 11;
    unsigned int dpid : 2;
    unsigned int pktsts : 4;
    unsigned int Reserved : 11;
  } b;
} USB_OTG_GRXFSTS_TypeDef ;
typedef union _USB_OTG_FSIZ_TypeDef {
  unsigned int d32;
  struct {
    unsigned int startaddr : 16;
    unsigned int depth : 16;
  } b;
} USB_OTG_FSIZ_TypeDef ;
typedef union _USB_OTG_HNPTXSTS_TypeDef {
  unsigned int d32;
  struct {
    unsigned int nptxfspcavail : 16;
    unsigned int nptxqspcavail : 8;
    unsigned int nptxqtop_terminate : 1;
    unsigned int nptxqtop_timer : 2;
    unsigned int nptxqtop : 2;
    unsigned int chnum : 2;
    unsigned int Reserved : 1;
  } b;
} USB_OTG_HNPTXSTS_TypeDef ;
typedef union _USB_OTG_DTXFSTSn_TypeDef {
  unsigned int d32;
  struct {
    unsigned int txfspcavail : 16;
    unsigned int Reserved : 16;
  } b;
} USB_OTG_DTXFSTSn_TypeDef ;
typedef union _USB_OTG_GI2CCTL_TypeDef {
  unsigned int d32;
  struct {
    unsigned int rwdata : 8;
    unsigned int regaddr : 8;
    unsigned int addr : 7;
    unsigned int i2cen : 1;
    unsigned int ack : 1;
    unsigned int i2csuspctl : 1;
    unsigned int i2cdevaddr : 2;
    unsigned int dat_se0: 1;
    unsigned int Reserved : 1;
    unsigned int rw : 1;
    unsigned int bsydne : 1;
  } b;
} USB_OTG_GI2CCTL_TypeDef ;
typedef union _USB_OTG_GCCFG_TypeDef {
  unsigned int d32;
  struct {
    unsigned int Reserved_in : 16;
    unsigned int pwdn : 1;
    unsigned int i2cifen : 1;
    unsigned int vbussensingA : 1;
    unsigned int vbussensingB : 1;
    unsigned int sofouten : 1;
    unsigned int disablevbussensing : 1;
    unsigned int Reserved_out : 10;
  } b;
} USB_OTG_GCCFG_TypeDef ;

typedef union _USB_OTG_DCFG_TypeDef {
  unsigned int d32;
  struct {
    unsigned int devspd : 2;
    unsigned int nzstsouthshk : 1;
    unsigned int Reserved3 : 1;
    unsigned int devaddr : 7;
    unsigned int perfrint : 2;
    unsigned int Reserved13_17 : 5;
    unsigned int epmscnt : 4;
  } b;
} USB_OTG_DCFG_TypeDef ;
typedef union _USB_OTG_DCTL_TypeDef {
  unsigned int d32;
  struct {
    unsigned int rmtwkupsig : 1;
    unsigned int sftdiscon : 1;
    unsigned int gnpinnaksts : 1;
    unsigned int goutnaksts : 1;
    unsigned int tstctl : 3;
    unsigned int sgnpinnak : 1;
    unsigned int cgnpinnak : 1;
    unsigned int sgoutnak : 1;
    unsigned int cgoutnak : 1;
    unsigned int Reserved : 21;
  } b;
} USB_OTG_DCTL_TypeDef ;
typedef union _USB_OTG_DSTS_TypeDef {
  unsigned int d32;
  struct {
    unsigned int suspsts : 1;
    unsigned int enumspd : 2;
    unsigned int errticerr : 1;
    unsigned int Reserved4_7: 4;
    unsigned int soffn : 14;
    unsigned int Reserved22_31 : 10;
  } b;
} USB_OTG_DSTS_TypeDef ;
typedef union _USB_OTG_DIEPINTn_TypeDef {
  unsigned int d32;
  struct {
    unsigned int xfercompl : 1;
    unsigned int epdisabled : 1;
    unsigned int ahberr : 1;
    unsigned int timeout : 1;
    unsigned int intktxfemp : 1;
    unsigned int intknepmis : 1;
    unsigned int inepnakeff : 1;
    unsigned int emptyintr : 1;
    unsigned int txfifoundrn : 1;
    unsigned int Reserved08_31 : 23;
  } b;
} USB_OTG_DIEPINTn_TypeDef ;
typedef union _USB_OTG_DIEPINTn_TypeDef   USB_OTG_DIEPMSK_TypeDef ;
typedef union _USB_OTG_DOEPINTn_TypeDef {
  unsigned int d32;
  struct {
    unsigned int xfercompl : 1;
    unsigned int epdisabled : 1;
    unsigned int ahberr : 1;
    unsigned int setup : 1;
    unsigned int Reserved04_31 : 28;
  } b;
} USB_OTG_DOEPINTn_TypeDef ;
typedef union _USB_OTG_DOEPINTn_TypeDef   USB_OTG_DOEPMSK_TypeDef ;

typedef union _USB_OTG_DAINT_TypeDef {
  unsigned int d32;
  struct {
    unsigned int in : 16;
    unsigned int out : 16;
  }
    ep;
} USB_OTG_DAINT_TypeDef ;

typedef union _USB_OTG_DTHRCTL_TypeDef {
  unsigned int d32;
  struct {
    unsigned int non_iso_thr_en : 1;
    unsigned int iso_thr_en : 1;
    unsigned int tx_thr_len : 9;
    unsigned int Reserved11_15 : 5;
    unsigned int rx_thr_en : 1;
    unsigned int rx_thr_len : 9;
    unsigned int Reserved26_31 : 6;
  } b;
} USB_OTG_DTHRCTL_TypeDef ;
typedef union _USB_OTG_DEPCTL_TypeDef {
  unsigned int d32;
  struct {
    unsigned int mps : 11;
    unsigned int reserved : 4;
    unsigned int usbactep : 1;
    unsigned int dpid : 1;
    unsigned int naksts : 1;
    unsigned int eptype : 2;
    unsigned int snp : 1;
    unsigned int stall : 1;
    unsigned int txfnum : 4;
    unsigned int cnak : 1;
    unsigned int snak : 1;
    unsigned int setd0pid : 1;
    unsigned int setd1pid : 1;
    unsigned int epdis : 1;
    unsigned int epena : 1;
  } b;
} USB_OTG_DEPCTL_TypeDef ;
typedef union _USB_OTG_DEPXFRSIZ_TypeDef {
  unsigned int d32;
  struct {
    unsigned int xfersize : 19;
    unsigned int pktcnt : 10;
    unsigned int mc : 2;
    unsigned int Reserved : 1;
  } b;
} USB_OTG_DEPXFRSIZ_TypeDef ;
typedef union _USB_OTG_DEP0XFRSIZ_TypeDef {
  unsigned int d32;
  struct {
    unsigned int xfersize : 7;
    unsigned int Reserved7_18 : 12;
    unsigned int pktcnt : 2;
    unsigned int Reserved20_28 : 9;
    unsigned int supcnt : 2;
    unsigned int Reserved31;
  } b;
} USB_OTG_DEP0XFRSIZ_TypeDef ;
typedef union _USB_OTG_HCFG_TypeDef {
  unsigned int d32;
  struct {
    unsigned int fslspclksel : 2;
    unsigned int fslssupp : 1;
  } b;
} USB_OTG_HCFG_TypeDef ;
typedef union _USB_OTG_HFRMINTRVL_TypeDef {
  unsigned int d32;
  struct {
    unsigned int frint : 16;
    unsigned int Reserved : 16;
  } b;
} USB_OTG_HFRMINTRVL_TypeDef ;

typedef union _USB_OTG_HFNUM_TypeDef {
  unsigned int d32;
  struct {
    unsigned int frnum : 16;
    unsigned int frrem : 16;
  } b;
} USB_OTG_HFNUM_TypeDef ;
typedef union _USB_OTG_HPTXSTS_TypeDef {
  unsigned int d32;
  struct {
    unsigned int ptxfspcavail : 16;
    unsigned int ptxqspcavail : 8;
    unsigned int ptxqtop_terminate : 1;
    unsigned int ptxqtop_timer : 2;
    unsigned int ptxqtop : 2;
    unsigned int chnum : 2;
    unsigned int ptxqtop_odd : 1;
  } b;
} USB_OTG_HPTXSTS_TypeDef ;
typedef union _USB_OTG_HPRT0_TypeDef {
  unsigned int d32;
  struct {
    unsigned int prtconnsts : 1;
    unsigned int prtconndet : 1;
    unsigned int prtena : 1;
    unsigned int prtenchng : 1;
    unsigned int prtovrcurract : 1;
    unsigned int prtovrcurrchng : 1;
    unsigned int prtres : 1;
    unsigned int prtsusp : 1;
    unsigned int prtrst : 1;
    unsigned int Reserved9 : 1;
    unsigned int prtlnsts : 2;
    unsigned int prtpwr : 1;
    unsigned int prttstctl : 4;
    unsigned int prtspd : 2;
    unsigned int Reserved19_31 : 13;
  } b;
} USB_OTG_HPRT0_TypeDef ;
typedef union _USB_OTG_HAINT_TypeDef {
  unsigned int d32;
  struct {
    unsigned int chint : 16;
    unsigned int Reserved : 16;
  } b;
} USB_OTG_HAINT_TypeDef ;
typedef union _USB_OTG_HAINTMSK_TypeDef {
  unsigned int d32;
  struct {
    unsigned int chint : 16;
    unsigned int Reserved : 16;
  } b;
} USB_OTG_HAINTMSK_TypeDef ;
typedef union _USB_OTG_HCCHAR_TypeDef {
  unsigned int d32;
  struct {
    unsigned int mps : 11;
    unsigned int epnum : 4;
    unsigned int epdir : 1;
    unsigned int Reserved : 1;
    unsigned int lspddev : 1;
    unsigned int eptype : 2;
    unsigned int multicnt : 2;
    unsigned int devaddr : 7;
    unsigned int oddfrm : 1;
    unsigned int chdis : 1;
    unsigned int chen : 1;
  } b;
} USB_OTG_HCCHAR_TypeDef ;
typedef union _USB_OTG_HCSPLT_TypeDef {
  unsigned int d32;
  struct {
    unsigned int prtaddr : 7;
    unsigned int hubaddr : 7;
    unsigned int xactpos : 2;
    unsigned int compsplt : 1;
    unsigned int Reserved : 14;
    unsigned int spltena : 1;
  } b;
} USB_OTG_HCSPLT_TypeDef ;
typedef union _USB_OTG_HCINTn_TypeDef {
  unsigned int d32;
  struct {
    unsigned int xfercompl : 1;
    unsigned int chhltd : 1;
    unsigned int ahberr : 1;
    unsigned int stall : 1;
    unsigned int nak : 1;
    unsigned int ack : 1;
    unsigned int nyet : 1;
    unsigned int xacterr : 1;
    unsigned int bblerr : 1;
    unsigned int frmovrun : 1;
    unsigned int datatglerr : 1;
    unsigned int Reserved : 21;
  } b;
} USB_OTG_HCINTn_TypeDef ;
typedef union _USB_OTG_HCTSIZn_TypeDef {
  unsigned int d32;
  struct {
    unsigned int xfersize : 19;
    unsigned int pktcnt : 10;
    unsigned int pid : 2;
    unsigned int dopng : 1;
  } b;
} USB_OTG_HCTSIZn_TypeDef ;
typedef union _USB_OTG_HCGINTMSK_TypeDef {
  unsigned int d32;
  struct {
    unsigned int xfercompl : 1;
    unsigned int chhltd : 1;
    unsigned int ahberr : 1;
    unsigned int stall : 1;
    unsigned int nak : 1;
    unsigned int ack : 1;
    unsigned int nyet : 1;
    unsigned int xacterr : 1;
    unsigned int bblerr : 1;
    unsigned int frmovrun : 1;
    unsigned int datatglerr : 1;
    unsigned int Reserved : 21;
  } b;
} USB_OTG_HCGINTMSK_TypeDef ;
typedef union _USB_OTG_PCGCCTL_TypeDef {
  unsigned int d32;
  struct {
    unsigned int stoppclk : 1;
    unsigned int gatehclk : 1;
    unsigned int Reserved : 30;
  } b;
} USB_OTG_PCGCCTL_TypeDef ;

#endif //usb_otg_regs_h
