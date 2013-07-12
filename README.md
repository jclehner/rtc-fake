Fake RTC for the Linux kernel
=============================

This driver's purpose is to provide hardware lacking an RTC with a method to set 
the time upon boot (when using a kernel with `CONFIG_RTC_HCTOSYS=y`). It was developed
with the Raspberry Pi in mind, to be used in conjunction with a tool similar to 
`swclock` (Gentoo, Arch Linux, etc.)or `fake-hwclock` (Debian, etc). Note that this
works only if the driver was _not_ built as a module, as it won't be loaded by the
time that `CONFIG_RTC_HCTOSYS` works its magic. 

For the fake RTC to become active, the `rtc-fake.time=` parameter must be a Unix timestamp 
other than zero. Thus the kernel may safely be compiled with `CONFIG_RTC_HCTOSYS=y` and 
`CONFIG_RTC_HCTOSYS_DEVICE=rtc0`: if a real RTC is available, not setting `rtc-fake.time` 
(or setting it to zero) will skip loading the fake driver, so `rtc0` may be used by the real
RTC.

When reading the time from the device it returns that time plus the amount of seconds elapsed 
since the device was initialized. Setting the time using a utility such as `hwclock` will fail, 
unless `rtc-fake.can_set_time=1` is specified (which doesn't really make sense, as it's lost 
upon reboot).


