#!/bin/bash
devname=xlg_dev
for ((i = 0;i<10;i++))
{

	sudo dd if=/dev/zero of=/dev/$devname bs=4k count=20000 oflag=direct&
}
