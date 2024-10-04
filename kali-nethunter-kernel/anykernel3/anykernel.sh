# AnyKernel2 Ramdisk Mod Script
# osm0sis @ xda-developers

## AnyKernel setup
# begin properties
properties() { '
kernel.string=Mad-Kali-MaxHunter by Duhjoker @ xda-developers
do.devicecheck=0
do.modules=0
do.cleanup=0
do.cleanuponabort=0
device.name1=crosshatch
device.name2=blueline
device.name3=bonito
device.name4=sargo
device.name5=
'; } # end properties

# shell variables
block=/dev/block/bootdevice/by-name/boot;
is_slot_device=1;
ramdisk_compression=auto;


## AnyKernel methods (DO NOT CHANGE)
# import patching functions/variables - see for reference
. /tmp/anykernel/tools/ak2-core.sh;


## AnyKernel file attributes
# set permissions/ownership for included ramdisk files
chmod -R 750 $ramdisk/*;
chmod -R 755 $ramdisk/sbin;
chown -R root:root $ramdisk/*;


## AnyKernel install
dump_boot;

if [ -d $ramdisk/.subackup -o -d $ramdisk/.backup ]; then
  patch_cmdline "skip_override" "skip_override";
else
  patch_cmdline "skip_override" "";
fi;

# Patch dtbo.img on custom ROMs
username="$(file_getprop /system/build.prop "ro.build.user")";
echo "Found user: $username";
case "$username" in
  "android-build") user=google;;
  *) user=custom;;
esac;
hostname="$(file_getprop /system/build.prop "ro.build.host")";
echo "Found host: $hostname";
case "$hostname" in
  *corp.google.com|abfarm*) host=google;;
  *) host=custom;;
esac;
if [ "$user" == "custom" -o "$host" == "custom" ]; then
  if [ ! -z /tmp/anykernel/dtbo.img ]; then
    ui_print " "; ui_print "You are on a custom ROM, patching kernel to remove verity...";
    $bin/magiskboot --dtb-patch /tmp/anykernel/dtbo.img;
    $bin/magiskboot --dtb-patch /tmp/anykernel/Image.lz4-dtb;
    
  fi;
else
  ui_print " "; ui_print "You are on stock, not patching kernel to remove verity!";
fi;

# begin ramdisk changes
# end ramdisk changes

write_boot;

## end install


