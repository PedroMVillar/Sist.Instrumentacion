# ============================================================
#  Genera el Utilization Report del frecuencimetro sin abrir la GUI.
#
#  Uso (parado en la carpeta que contiene los .sv y el .xdc):
#     vivado -mode batch -source reporte_utilizacion.tcl
#
#  Deja el resultado en  utilizacion.rpt  (y tambien lo imprime en pantalla).
# ============================================================

set part      xc7s50csga324-1
set outdir    [pwd]
set srcdir    [pwd]

create_project -in_memory -part $part

read_verilog -sv [list \
    $srcdir/BaseTime1ms.sv        \
    $srcdir/BCD_counter.sv        \
    $srcdir/BCD_bank.sv           \
    $srcdir/Latch_register.sv     \
    $srcdir/FSM_Control.sv        \
    $srcdir/7seg_controller.sv    \
    $srcdir/Frequency_meter.sv    \
]

read_xdc $srcdir/boolean.xdc

synth_design -top Frequency_meter -part $part
opt_design
place_design
route_design

report_utilization -file $outdir/utilizacion.rpt
report_utilization

puts "\n>>> Listo. Reporte escrito en $outdir/utilizacion.rpt"
