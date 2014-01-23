import os

FRAMES = 24

os.mkdir("3d_test")
for i in range(0, FRAMES):
    os.system("cat LR1.svg | sed -e \"s/L1/L%d/\" | sed -e \"s/R1/R%d/\" > tmp.svg" % (i + 1, i + 1))
    os.system("inkscape --export-png=3d_test/%06d.png --export-area-page tmp.svg" % (i + 1))
