#!/usr/bin/env python

import struct
from binascii import hexlify
from construct import BitStruct, BitField, Padding, Enum, Flag
from pyrsp.rsp import CortexM3
from pyrsp.utils import unhex, switch_endian
import capstone as cs
import sys

SCB_VTOR = 0xe000ed08

SCB_ICSR = 0xe000ed04
scb_icsr = BitStruct("scb_icsr",
                     Flag('NMIPENDSET'),
                     Padding(2),
                     Flag('PENDSVSET'),
                     Flag('PENDSVCLR'),
                     Flag('PENDSTSET'),
                     Flag('PENDSTCLR'),
                     Padding(2),
                     #Padding(1),
                     #Flag('DEBUG'),
                     Flag('ISRPENDING'),
                     BitField('VECTPENDING', 10),
                     Flag('RETOBASE'),
                     Padding(2),
                     BitField('VECTACTIVE', 9),
                     )

SCB_SHCSR = 0xe000ed24
scb_shcsr = BitStruct("scb_shcsr",
                     Padding(13),
                      Flag('USGFAULTENA'),
                      Flag('BUSFAULTENA'),
                      Flag('MEMFAULTENA'),
                      Flag('SVCALLPENDED'),
                      Flag('BUSFAULTPENDED'),
                      Flag('MEMFAULTPENDED'),
                      Flag('USGFAULTPENDED'),
                      Flag('SYSTICKACT'),
                      Flag('PENDSVACT'),
                      Padding(1),
                      Flag('MONITORACT'),
                      Flag('SVCALLACT'),
                      Padding(3),
                      Flag('USGFAULTACT'),
                      Padding(1),
                      Flag('BUSFAULTACT'),
                      Flag('MEMFAULTACT'))

SCB_CFSR = 0xe000ed28
scb_cfsr = BitStruct("scb_cfsr",
                     Padding(6),
                     Flag("DIVBYZERO"),
                     Flag("UNALIGNED"),
                     Padding(4),
                     Flag("NOCP"),
                     Flag("INVPC"),
                     Flag("INVSTATE"),
                     Flag("UNDEFINSTR"),
                     Flag("BFARVALID"),
                     Padding(2),
                     Flag("STKERR"),
                     Flag("UNSTKERR"),
                     Flag("IMPRECISERR"),
                     Flag("PRECISERR"),
                     Flag("IBUSERR"),
                     Flag("MMARVALID"),
                     Padding(2),
                     Flag("MSTKERR"),
                     Flag("MUNSTKERR"),
                     Padding(1),
                     Flag("DACCVIOL"),
                     Flag("IACCVIOL"),
                     )

SCB_HFSR = 0xe000ed2c
scb_hfsr = BitStruct("scb_hfsr",
                     Flag("DEBUG_VT"),
                     Flag("FORCED"),
                     Padding(28),
                     Flag("VECTTBL"),
                     Padding(1),
                     )
SCB_MMFAR = 0xe000ed34
SCB_BFAR = 0xe000ed38

MPU_TYPER = 0xe000ed90
MPU_CR = 0xe000ed94
mpu_cr = BitStruct("mpu_cr",
                   Padding(29),
                   Flag("PRIVDEFENA"),
                   Flag("HFNMIENA"),
                   Flag("ENABLE"),
                   )

MPU_RNR = 0xe000ed98
mpu_rnr = BitStruct("mpu_rnr",
                    Padding(24),
                    BitField('REGION', 8))

MPU_RBAR = 0xe000ed9c
mpu_rbar = BitStruct("mpu_rbar",
                     BitField('ADDR', 27),
                     Flag('VALID'),
                     BitField('REGION', 4))

MPU_RASR = 0xe000eda0
mpu_rasr = BitStruct("mpu_rasr",
                     Padding(3),
                     Flag("XN"),
                     Padding(1),
                     #BitField("AP", 3),
                     Enum(BitField("AP", 3),
                          No_access = 0,
                          RW_No_access = 1,
                          RW_RO = 2,
                          RW = 3,
                          RO_No_access = 5,
                          RO = 6,
                          INV7 = 7),
                     Padding(2),
                     #BitField("TEX", 3), #Flag("S", 1), #Flag("C", 1), #Flag("B", 1),
                     Enum(BitField("TEXSCB", 6),
                          UNSET   =(0b000000),
                          FLASH_RAM   =(0b000010),
                          INTERNAL_RAM=(0b000110),
                          EXTERNAL_RAM=(0b000111),
                          PERIPHERIALS=(0b000101),),
                     BitField("SRD", 8),
                     Padding(2),
                     #BitField("SIZE", 5),
                     Enum(BitField("SIZE", 5),
                         unset   =(0b00000), invalid1=(0b00001), invalid2=(0b00010), invalid3=(0b00011),
                         _32B    =(0b00100), _64B    =(0b00101), _128B   =(0b00110), _256B   =(0b00111),
                         _512B   =(0b01000), _1KB    =(0b01001), _2KB    =(0b01010), _4KB    =(0b01011),
                         _8KB    =(0b01100), _16KB   =(0b01101), _32KB   =(0b01110), _64KB   =(0b01111),
                         _128KB  =(0b10000), _256KB  =(0b10001), _512KB  =(0b10010), _1MB    =(0b10011),
                         _2MB    =(0b10100), _4MB    =(0b10101), _8MB    =(0b10110), _16MB   =(0b10111),
                         _32MB   =(0b11000), _64MB   =(0b11001), _128MB  =(0b11010), _256MB  =(0b11011),
                         _512MB  =(0b11100), _1GB    =(0b11101), _2GB    =(0b11110), _4GB    =(0b11111),
                     ),
                     Flag("Enabled"))

FLASH_OPTCR = (((0x40000000 + 0x20000) + 0x3C00) + 0x14)
flash_optcr = BitStruct("flash_optcr",
                        Padding(4),
                        BitField('nWRP', 12),
                        #BitField('RDP', 8),
                        Enum(BitField("RDP", 8),
                             _default_ = "level1",
                             level0 = 0xaa,
                             level2 = 0xcc),
                        BitField('USER', 3),
                        Padding(1),
                        BitField('BOR_LEV', 2),
                        Flag('OPTSTRT'),
                        Flag('OPTLOCK'))

xpsr = BitStruct("program status register",
                Flag("negative"),
                Flag("zero"),
                Flag("carry"),
                Flag("overflow"),
                Flag("sticky saturation"),

                Padding(19), # ignoring espr

                BitField('isr number', 8))

def getreg(rsp,size,ptr):
    tmp = rsp.fetch('m%x,%x' % (ptr, size))
    return unhex(switch_endian(tmp))

def printreg(reg):
    return [n if type(v)==bool and v==True else (n,v if n!='ADDR' else hex(v<<5)) for n,v in reg.items() if v]

def dump_mpu(rsp):
    print 'mpu_cr', printreg(mpu_cr.parse(getreg(rsp, 4, MPU_CR)))
    for region in xrange(8):
        rsp.store(struct.pack("<I", region), MPU_RNR)
        print region,
        print printreg(mpu_rbar.parse(getreg(rsp, 4, MPU_RBAR))),
        print printreg(mpu_rasr.parse(getreg(rsp, 4, MPU_RASR)))

def check_fault(rsp):
    print 'hfsr=', printreg(scb_hfsr.parse(getreg(rsp, 4, SCB_HFSR)))
    print 'icsr=', printreg(scb_icsr.parse(getreg(rsp, 4, SCB_ICSR)))
    print 'shcsr=', printreg(scb_shcsr.parse(getreg(rsp, 4, SCB_SHCSR)))
    print 'cfsr=', printreg(scb_cfsr.parse(getreg(rsp, 4, SCB_CFSR)))
    print 'MMFAR=', hex(struct.unpack(">I", getreg(rsp, 4, SCB_MMFAR))[0])
    print 'BFAR=', hex(struct.unpack(">I", getreg(rsp, 4, SCB_BFAR))[0])

def disassm(rsp, addr, epsilon):
    code=rsp.dump(2+2*epsilon,addr-epsilon)
    md=cs.Cs(cs.CS_ARCH_ARM, cs.CS_MODE_THUMB)
    idx=0
    for (address, size, mnemonic, op_str) in md.disasm_lite(code, addr-epsilon):
        src_line = rsp.get_src_line(address)
        if address==int(rsp.regs['pc'],16) and epsilon>0:
            print "0x%x: >\t%s\t%s\t%s" %(address, hexlify(code[idx:idx+size]), mnemonic, op_str),
        else:
            print "0x%x:\t%s\t%s\t%s" %(address, hexlify(code[idx:idx+size]), mnemonic, op_str),
        if src_line:
            print ";\t\t %s:%s %s" % (src_line['file'], src_line['lineno'], src_line['line'])
        else:
            print
        idx+=size

def trace(rsp, steps=20000):
    while(steps>0):
        #print "%5d" % steps,
        disassm(rsp, int(rsp.regs['pc'],16), 0)
        r = rsp.fetch('s')
        if r != 'T05': print '[w]', r
        rsp.refresh_regs()
        steps-=1

if __name__ == "__main__":
    try:
        elffile=sys.argv[1] if len(sys.argv)>1 else None
        rsp = CortexM3('/dev/ttyACM0', elffile, verbose=False)
        rsp.refresh_regs()
        dump_mpu(rsp)
        print 'flash_optcr=', printreg(flash_optcr.parse(getreg(rsp, 4, FLASH_OPTCR)))
        check_fault(rsp)
        print 'VTOR=', hex(struct.unpack(">I", getreg(rsp, 4, SCB_VTOR))[0])
        rsp.lazy_dump_regs()
        print 'xpsr=', printreg(xpsr.parse(unhex(rsp.regs['xpsr'])))
        #print hex()
        disassm(rsp, int(rsp.regs['pc'],16), 4)
        if int(rsp.regs['pc'],16) in [ 0x8003422, 0x8003424, 0x8003426]:
            rsp.set_reg('r3',rsp.regs['r0'])
            print "[x] force stepping over loop"
            while int(rsp.regs['pc'],16) < 0x8003426:
                rsp.lazy_dump_regs()
                r = rsp.fetch('s')
                if r != 'T05': print r
            disassm(rsp, int(rsp.regs['pc'],16), 4)

        print "[x] tracing"
        trace(rsp, 20)
        rsp.port.close(rsp)

    # this is for cleaning up in case a keyboard interrupt came
    except KeyboardInterrupt:
        import traceback
        traceback.print_exc()

        res = None
        while not res:
            res = rsp.port.read()
        discards = []
        retries = 20
        while res!='+' and retries>0:
            discards.append(res)
            retries-=1
            res = rsp.port.read()
        if len(discards)>0 and rsp.verbose: print 'send discards', discards

        rsp.port.close(rsp)
        sys.exit(1)
