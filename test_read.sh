#!/bin/bash
devname=xlg_dev
for ((i = 0;i < 10;i++))
{
	sudo dd if=/dev/$devname of=/dev/null bs=4k count=20000 iflag=direct&
}
