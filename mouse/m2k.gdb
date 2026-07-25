set pagination off
target extended-remote localhost:3333
file m2k.elf
monitor reset halt

define zk_config_print_flash
  x/64x 0x8004000
end
