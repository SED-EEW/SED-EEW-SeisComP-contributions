# Tests

# testeewampsprocs

Tests the basic processing domains. The main purpose is to test the base layer
which includes degain, baseline correction and combination of horizontal channels.
Furthermore envelopes are being calculated. The ```--dump``` option writes out
all processed data as MiniSEED to stdout. To distinguish the different processing
steps the following channel naming conventions are being used:

- channel ??X: combined horizontal channel
- location PA/PV: virtual co-located channel, either acceleration or velocity
- location PD: displacement channel
- location EA: acceleration envelopes
- location EV: velocity envelopes
- location ED: displacement envelopes

Test run:

```
testeewampsprocs -I IU.COLA-test.mseed -d tesla --debug --dump > test.mseed
```

The filterbank test has to be run using `faketime`:

```
faketime -f "@1996-08-10 18:12:22.82" testgba --inventory-db Inventory_BO_ABK.xml --debug -I test2.mseed.sorted > fb_AKT019.txt
```
