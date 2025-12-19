import re
import pandas as pd

# Paste your raw log data here as a multiline string
log_data = """
I (405) INA219: Shunt Voltage: 3.61 mV
I (405) INA219: Current: 0.036100 A
I (405) INA219: Power: 0.432000 W
I (415) INA3221: C1: Bus Voltage: 11.98 V
I (415) INA3221: C1: Shunt Voltage: 1.32 mV
I (415) INA3221: C1: Shunt Current: 13.20 mA

I (425) INA3221: C2: Bus Voltage: 11.98 V
I (425) INA3221: C2: Shunt Voltage: 1.32 mV
I (435) INA3221: C2: Shunt Current: 13.20 mA

I (435) INA3221: C3: Bus Voltage: 11.98 V
I (435) INA3221: C3: Shunt Voltage: 1.32 mV
I (445) INA3221: C3: Shunt Current: 13.20 mA

I (1415) INA219: Bus Voltage: 12.01 V
I (1415) INA219: Shunt Voltage: 3.61 mV
I (1415) INA219: Current: 0.036100 A
I (1415) INA219: Power: 0.432000 W
I (1945) INA3221: C1: Bus Voltage: 11.98 V
I (1945) INA3221: C1: Shunt Voltage: 1.32 mV
I (1945) INA3221: C1: Shunt Current: 13.20 mA

I (1945) INA3221: C2: Bus Voltage: 11.98 V
I (1945) INA3221: C2: Shunt Voltage: 1.32 mV
I (1955) INA3221: C2: Shunt Current: 13.20 mA

I (1955) INA3221: C3: Bus Voltage: 11.98 V
I (1955) INA3221: C3: Shunt Voltage: 1.32 mV
I (1965) INA3221: C3: Shunt Current: 13.20 mA

I (2415) INA219: Bus Voltage: 12.01 V
I (2415) INA219: Shunt Voltage: 3.61 mV
I (2415) INA219: Current: 0.036100 A
I (2415) INA219: Power: 0.432000 W
I (3415) INA219: Bus Voltage: 12.01 V
I (3415) INA219: Shunt Voltage: 3.61 mV
I (3415) INA219: Current: 0.036100 A
I (3415) INA219: Power: 0.432000 W
I (3465) INA3221: C1: Bus Voltage: 11.98 V
I (3465) INA3221: C1: Shunt Voltage: 1.32 mV
I (3465) INA3221: C1: Shunt Current: 13.20 mA

I (3465) INA3221: C2: Bus Voltage: 11.98 V
I (3465) INA3221: C2: Shunt Voltage: 1.32 mV
I (3475) INA3221: C2: Shunt Current: 13.20 mA

I (3475) INA3221: C3: Bus Voltage: 11.98 V
I (3475) INA3221: C3: Shunt Voltage: 1.32 mV
I (3485) INA3221: C3: Shunt Current: 13.20 mA

I (4415) INA219: Bus Voltage: 12.01 V
I (4415) INA219: Shunt Voltage: 3.55 mV
I (4415) INA219: Current: 0.035500 A
I (4415) INA219: Power: 0.426000 W
I (4985) INA3221: C1: Bus Voltage: 11.98 V
I (4985) INA3221: C1: Shunt Voltage: 1.32 mV
I (4985) INA3221: C1: Shunt Current: 13.20 mA

I (4985) INA3221: C2: Bus Voltage: 11.98 V
I (4985) INA3221: C2: Shunt Voltage: 1.32 mV
I (4995) INA3221: C2: Shunt Current: 13.20 mA

I (4995) INA3221: C3: Bus Voltage: 11.98 V
I (4995) INA3221: C3: Shunt Voltage: 1.24 mV
I (5005) INA3221: C3: Shunt Current: 12.40 mA

I (5415) INA219: Bus Voltage: 12.01 V
I (5415) INA219: Shunt Voltage: 3.51 mV
I (5415) INA219: Current: 0.035100 A
I (5415) INA219: Power: 0.422000 W
I (6415) INA219: Bus Voltage: 12.01 V
I (6415) INA219: Shunt Voltage: 3.51 mV
I (6415) INA219: Current: 0.035100 A
I (6415) INA219: Power: 0.422000 W
I (6505) INA3221: C1: Bus Voltage: 11.98 V
I (6505) INA3221: C1: Shunt Voltage: 1.32 mV
I (6505) INA3221: C1: Shunt Current: 13.20 mA

I (6505) INA3221: C2: Bus Voltage: 11.98 V
I (6505) INA3221: C2: Shunt Voltage: 1.32 mV
I (6515) INA3221: C2: Shunt Current: 13.20 mA

I (6515) INA3221: C3: Bus Voltage: 11.98 V
I (6515) INA3221: C3: Shunt Voltage: 1.20 mV
I (6525) INA3221: C3: Shunt Current: 12.00 mA

I (7415) INA219: Bus Voltage: 12.01 V
I (7415) INA219: Shunt Voltage: 3.41 mV
I (7415) INA219: Current: 0.034100 A
I (7415) INA219: Power: 0.410000 W
I (8025) INA3221: C1: Bus Voltage: 11.98 V
I (8025) INA3221: C1: Shunt Voltage: 1.32 mV
I (8025) INA3221: C1: Shunt Current: 13.20 mA

I (8025) INA3221: C2: Bus Voltage: 11.98 V
I (8025) INA3221: C2: Shunt Voltage: 1.24 mV
I (8035) INA3221: C2: Shunt Current: 12.40 mA

I (8035) INA3221: C3: Bus Voltage: 11.98 V
I (8035) INA3221: C3: Shunt Voltage: 1.20 mV
I (8045) INA3221: C3: Shunt Current: 12.00 mA

I (8415) INA219: Bus Voltage: 12.01 V
I (8415) INA219: Shunt Voltage: 3.38 mV
I (8415) INA219: Current: 0.033800 A
I (8415) INA219: Power: 0.406000 W
I (9415) INA219: Bus Voltage: 12.01 V
I (9415) INA219: Shunt Voltage: 3.38 mV
I (9415) INA219: Current: 0.033800 A
I (9415) INA219: Power: 0.406000 W
I (9545) INA3221: C1: Bus Voltage: 11.98 V
I (9545) INA3221: C1: Shunt Voltage: 1.32 mV
I (9545) INA3221: C1: Shunt Current: 13.20 mA

I (9545) INA3221: C2: Bus Voltage: 11.98 V
I (9545) INA3221: C2: Shunt Voltage: 1.20 mV
I (9555) INA3221: C2: Shunt Current: 12.00 mA

I (9555) INA3221: C3: Bus Voltage: 11.98 V
I (9555) INA3221: C3: Shunt Voltage: 1.20 mV
I (9565) INA3221: C3: Shunt Current: 12.00 mA

I (10415) INA219: Bus Voltage: 12.01 V
I (10415) INA219: Shunt Voltage: 3.31 mV
I (10415) INA219: Current: 0.033100 A
I (10415) INA219: Power: 0.396000 W
I (11065) INA3221: C1: Bus Voltage: 11.98 V
I (11065) INA3221: C1: Shunt Voltage: 1.28 mV
I (11065) INA3221: C1: Shunt Current: 12.80 mA

I (11065) INA3221: C2: Bus Voltage: 11.99 V
I (11065) INA3221: C2: Shunt Voltage: 1.20 mV
I (11075) INA3221: C2: Shunt Current: 12.00 mA

I (11075) INA3221: C3: Bus Voltage: 11.98 V
I (11075) INA3221: C3: Shunt Voltage: 1.20 mV
I (11085) INA3221: C3: Shunt Current: 12.00 mA

I (11415) INA219: Bus Voltage: 12.01 V
I (11415) INA219: Shunt Voltage: 3.28 mV
I (11415) INA219: Current: 0.032800 A
I (11415) INA219: Power: 0.394000 W
I (12415) INA219: Bus Voltage: 12.01 V
I (12415) INA219: Shunt Voltage: 3.28 mV
I (12415) INA219: Current: 0.032800 A
I (12415) INA219: Power: 0.394000 W
I (12585) INA3221: C1: Bus Voltage: 11.98 V
I (12585) INA3221: C1: Shunt Voltage: 1.24 mV
I (12585) INA3221: C1: Shunt Current: 12.40 mA

I (12585) INA3221: C2: Bus Voltage: 11.99 V
I (12585) INA3221: C2: Shunt Voltage: 1.20 mV
I (12595) INA3221: C2: Shunt Current: 12.00 mA

I (12595) INA3221: C3: Bus Voltage: 11.98 V
I (12595) INA3221: C3: Shunt Voltage: 1.20 mV
I (12605) INA3221: C3: Shunt Current: 12.00 mA

I (13415) INA219: Bus Voltage: 12.01 V
I (13415) INA219: Shunt Voltage: 3.28 mV
I (13415) INA219: Current: 0.032800 A
I (13415) INA219: Power: 0.394000 W
I (14105) INA3221: C1: Bus Voltage: 11.98 V
I (14105) INA3221: C1: Shunt Voltage: 1.24 mV
I (14105) INA3221: C1: Shunt Current: 12.40 mA

I (14105) INA3221: C2: Bus Voltage: 11.99 V
I (14105) INA3221: C2: Shunt Voltage: 1.20 mV
I (14115) INA3221: C2: Shunt Current: 12.00 mA

I (14115) INA3221: C3: Bus Voltage: 11.98 V
I (14115) INA3221: C3: Shunt Voltage: 1.20 mV
I (14125) INA3221: C3: Shunt Current: 12.00 mA

I (14415) INA219: Bus Voltage: 12.01 V
I (14415) INA219: Shunt Voltage: 3.28 mV
I (14415) INA219: Current: 0.032800 A
I (14415) INA219: Power: 0.394000 W
I (15315) SYSTEM_MONITOR: Free heap: 283416 bytes
I (15415) INA219: Bus Voltage: 12.01 V
I (15415) INA219: Shunt Voltage: 3.28 mV
I (15415) INA219: Current: 0.032800 A
I (15415) INA219: Power: 0.394000 W
I (15625) INA3221: C1: Bus Voltage: 11.98 V
I (15625) INA3221: C1: Shunt Voltage: 1.24 mV
I (15625) INA3221: C1: Shunt Current: 12.40 mA

I (15625) INA3221: C2: Bus Voltage: 11.98 V
I (15625) INA3221: C2: Shunt Voltage: 1.20 mV
I (15635) INA3221: C2: Shunt Current: 12.00 mA

I (15635) INA3221: C3: Bus Voltage: 11.98 V
I (15635) INA3221: C3: Shunt Voltage: 1.20 mV
I (15645) INA3221: C3: Shunt Current: 12.00 mA

I (16415) INA219: Bus Voltage: 12.01 V
I (16415) INA219: Shunt Voltage: 3.31 mV
I (16415) INA219: Current: 0.033100 A
I (16415) INA219: Power: 0.396000 W
I (17145) INA3221: C1: Bus Voltage: 11.98 V
I (17145) INA3221: C1: Shunt Voltage: 1.24 mV
I (17145) INA3221: C1: Shunt Current: 12.40 mA

I (17145) INA3221: C2: Bus Voltage: 11.98 V
I (17145) INA3221: C2: Shunt Voltage: 1.20 mV
I (17155) INA3221: C2: Shunt Current: 12.00 mA

I (17155) INA3221: C3: Bus Voltage: 11.98 V
I (17155) INA3221: C3: Shunt Voltage: 1.36 mV
I (17165) INA3221: C3: Shunt Current: 13.60 mA

I (17415) INA219: Bus Voltage: 12.01 V
I (17415) INA219: Shunt Voltage: 3.71 mV
I (17415) INA219: Current: 0.037100 A
I (17415) INA219: Power: 0.446000 W
I (18415) INA219: Bus Voltage: 12.01 V
I (18415) INA219: Shunt Voltage: 3.69 mV
I (18415) INA219: Current: 0.036900 A
I (18415) INA219: Power: 0.442000 W
I (18665) INA3221: C1: Bus Voltage: 11.98 V
I (18665) INA3221: C1: Shunt Voltage: 1.24 mV
I (18665) INA3221: C1: Shunt Current: 12.40 mA

I (18665) INA3221: C2: Bus Voltage: 11.98 V
I (18665) INA3221: C2: Shunt Voltage: 1.20 mV
I (18675) INA3221: C2: Shunt Current: 12.00 mA

I (18675) INA3221: C3: Bus Voltage: 11.98 V
I (18675) INA3221: C3: Shunt Voltage: 1.56 mV
I (18685) INA3221: C3: Shunt Current: 15.60 mA

I (19415) INA219: Bus Voltage: 12.00 V
I (19415) INA219: Shunt Voltage: 3.87 mV
I (19415) INA219: Current: 0.038700 A
I (19415) INA219: Power: 0.466000 W
I (20185) INA3221: C1: Bus Voltage: 11.98 V
I (20185) INA3221: C1: Shunt Voltage: 1.24 mV
I (20185) INA3221: C1: Shunt Current: 12.40 mA

I (20185) INA3221: C2: Bus Voltage: 11.98 V
I (20185) INA3221: C2: Shunt Voltage: 1.52 mV
I (20195) INA3221: C2: Shunt Current: 15.20 mA

I (20195) INA3221: C3: Bus Voltage: 11.98 V
I (20195) INA3221: C3: Shunt Voltage: 1.60 mV
I (20205) INA3221: C3: Shunt Current: 16.00 mA

I (20415) INA219: Bus Voltage: 12.00 V
I (20415) INA219: Shunt Voltage: 4.18 mV
I (20415) INA219: Current: 0.041800 A
I (20415) INA219: Power: 0.502000 W
I (21415) INA219: Bus Voltage: 12.00 V
I (21415) INA219: Shunt Voltage: 4.21 mV
I (21415) INA219: Current: 0.042100 A
I (21415) INA219: Power: 0.504000 W
I (21705) INA3221: C1: Bus Voltage: 11.98 V
I (21705) INA3221: C1: Shunt Voltage: 1.24 mV
I (21705) INA3221: C1: Shunt Current: 12.40 mA

I (21705) INA3221: C2: Bus Voltage: 11.98 V
I (21705) INA3221: C2: Shunt Voltage: 1.64 mV
I (21715) INA3221: C2: Shunt Current: 16.40 mA

I (21715) INA3221: C3: Bus Voltage: 11.98 V
I (21715) INA3221: C3: Shunt Voltage: 1.60 mV
I (21725) INA3221: C3: Shunt Current: 16.00 mA

I (22415) INA219: Bus Voltage: 12.00 V
I (22415) INA219: Shunt Voltage: 4.31 mV
I (22415) INA219: Current: 0.043100 A
I (22415) INA219: Power: 0.514000 W
I (23225) INA3221: C1: Bus Voltage: 11.98 V
I (23225) INA3221: C1: Shunt Voltage: 1.52 mV
I (23225) INA3221: C1: Shunt Current: 15.20 mA

I (23225) INA3221: C2: Bus Voltage: 11.98 V
I (23225) INA3221: C2: Shunt Voltage: 1.68 mV
I (23235) INA3221: C2: Shunt Current: 16.80 mA

I (23235) INA3221: C3: Bus Voltage: 11.98 V
I (23235) INA3221: C3: Shunt Voltage: 1.60 mV
I (23245) INA3221: C3: Shunt Current: 16.00 mA

I (23415) INA219: Bus Voltage: 12.00 V
I (23415) INA219: Shunt Voltage: 4.60 mV
I (23415) INA219: Current: 0.046000 A
I (23415) INA219: Power: 0.550000 W
I (24415) INA219: Bus Voltage: 12.00 V
I (24415) INA219: Shunt Voltage: 4.60 mV
I (24415) INA219: Current: 0.046000 A
I (24415) INA219: Power: 0.550000 W
I (24745) INA3221: C1: Bus Voltage: 11.98 V
I (24745) INA3221: C1: Shunt Voltage: 1.60 mV
I (24745) INA3221: C1: Shunt Current: 16.00 mA

I (24745) INA3221: C2: Bus Voltage: 11.98 V
I (24745) INA3221: C2: Shunt Voltage: 1.68 mV
I (24755) INA3221: C2: Shunt Current: 16.80 mA

I (24755) INA3221: C3: Bus Voltage: 11.98 V
I (24755) INA3221: C3: Shunt Voltage: 1.60 mV
I (24765) INA3221: C3: Shunt Current: 16.00 mA

I (25415) INA219: Bus Voltage: 12.00 V
I (25415) INA219: Shunt Voltage: 4.61 mV
I (25415) INA219: Current: 0.046100 A
I (25415) INA219: Power: 0.554000 W
I (26265) INA3221: C1: Bus Voltage: 11.98 V
I (26265) INA3221: C1: Shunt Voltage: 1.64 mV
I (26265) INA3221: C1: Shunt Current: 16.40 mA

I (26265) INA3221: C2: Bus Voltage: 11.98 V
I (26265) INA3221: C2: Shunt Voltage: 1.68 mV
I (26275) INA3221: C2: Shunt Current: 16.80 mA

I (26275) INA3221: C3: Bus Voltage: 11.98 V
I (26275) INA3221: C3: Shunt Voltage: 1.60 mV
I (26285) INA3221: C3: Shunt Current: 16.00 mA

I (26415) INA219: Bus Voltage: 12.00 V
I (26415) INA219: Shunt Voltage: 4.61 mV
I (26415) INA219: Current: 0.046100 A
I (26415) INA219: Power: 0.554000 W
I (27415) INA219: Bus Voltage: 12.00 V
I (27415) INA219: Shunt Voltage: 4.61 mV
I (27415) INA219: Current: 0.046100 A
I (27415) INA219: Power: 0.554000 W
I (27785) INA3221: C1: Bus Voltage: 11.98 V
I (27785) INA3221: C1: Shunt Voltage: 1.64 mV
I (27785) INA3221: C1: Shunt Current: 16.40 mA

I (27785) INA3221: C2: Bus Voltage: 11.98 V
I (27785) INA3221: C2: Shunt Voltage: 1.68 mV
I (27795) INA3221: C2: Shunt Current: 16.80 mA

I (27795) INA3221: C3: Bus Voltage: 11.98 V
I (27795) INA3221: C3: Shunt Voltage: 1.60 mV
I (27805) INA3221: C3: Shunt Current: 16.00 mA

I (28415) INA219: Bus Voltage: 12.00 V
I (28415) INA219: Shunt Voltage: 4.61 mV
I (28415) INA219: Current: 0.046100 A
I (28415) INA219: Power: 0.554000 W
I (29305) INA3221: C1: Bus Voltage: 11.98 V
I (29305) INA3221: C1: Shunt Voltage: 1.64 mV
I (29305) INA3221: C1: Shunt Current: 16.40 mA

I (29305) INA3221: C2: Bus Voltage: 11.98 V
I (29305) INA3221: C2: Shunt Voltage: 1.40 mV
I (29315) INA3221: C2: Shunt Current: 14.00 mA

I (29315) INA3221: C3: Bus Voltage: 11.98 V
I (29315) INA3221: C3: Shunt Voltage: 1.60 mV
I (29325) INA3221: C3: Shunt Current: 16.00 mA

I (29415) INA219: Bus Voltage: 12.00 V
I (29415) INA219: Shunt Voltage: 4.61 mV
I (29415) INA219: Current: 0.046100 A
I (29415) INA219: Power: 0.554000 W
I (30315) SYSTEM_MONITOR: Free heap: 283416 bytes
I (30415) INA219: Bus Voltage: 12.00 V
I (30415) INA219: Shunt Voltage: 4.61 mV
I (30415) INA219: Current: 0.046100 A
I (30415) INA219: Power: 0.554000 W
I (30825) INA3221: C1: Bus Voltage: 11.98 V
I (30825) INA3221: C1: Shunt Voltage: 1.64 mV
I (30825) INA3221: C1: Shunt Current: 16.40 mA

I (30825) INA3221: C2: Bus Voltage: 11.98 V
I (30825) INA3221: C2: Shunt Voltage: 1.64 mV
I (30835) INA3221: C2: Shunt Current: 16.40 mA

I (30835) INA3221: C3: Bus Voltage: 11.98 V
I (30835) INA3221: C3: Shunt Voltage: 1.60 mV
I (30845) INA3221: C3: Shunt Current: 16.00 mA

I (31415) INA219: Bus Voltage: 12.00 V
I (31415) INA219: Shunt Voltage: 4.61 mV
I (31415) INA219: Current: 0.046100 A
I (31415) INA219: Power: 0.550000 W
I (32345) INA3221: C1: Bus Voltage: 11.98 V
I (32345) INA3221: C1: Shunt Voltage: 1.64 mV
I (32345) INA3221: C1: Shunt Current: 16.40 mA

I (32345) INA3221: C2: Bus Voltage: 11.98 V
I (32345) INA3221: C2: Shunt Voltage: 1.68 mV
I (32355) INA3221: C2: Shunt Current: 16.80 mA

I (32355) INA3221: C3: Bus Voltage: 11.98 V
I (32355) INA3221: C3: Shunt Voltage: 1.60 mV
I (32365) INA3221: C3: Shunt Current: 16.00 mA

I (32415) INA219: Bus Voltage: 12.00 V
I (32415) INA219: Shunt Voltage: 4.61 mV
I (32415) INA219: Current: 0.046100 A
I (32415) INA219: Power: 0.554000 W
I (33415) INA219: Bus Voltage: 12.00 V
I (33415) INA219: Shunt Voltage: 4.61 mV
I (33415) INA219: Current: 0.046100 A
I (33415) INA219: Power: 0.554000 W
I (33865) INA3221: C1: Bus Voltage: 11.98 V
I (33865) INA3221: C1: Shunt Voltage: 1.64 mV
I (33865) INA3221: C1: Shunt Current: 16.40 mA

I (33865) INA3221: C2: Bus Voltage: 11.98 V
I (33865) INA3221: C2: Shunt Voltage: 1.68 mV
I (33875) INA3221: C2: Shunt Current: 16.80 mA

I (33875) INA3221: C3: Bus Voltage: 11.98 V
I (33875) INA3221: C3: Shunt Voltage: 1.60 mV
I (33885) INA3221: C3: Shunt Current: 16.00 mA

I (34415) INA219: Bus Voltage: 12.00 V
I (34415) INA219: Shunt Voltage: 4.61 mV
I (34415) INA219: Current: 0.046100 A
I (34415) INA219: Power: 0.554000 W
I (35385) INA3221: C1: Bus Voltage: 11.98 V
I (35385) INA3221: C1: Shunt Voltage: 1.64 mV
I (35385) INA3221: C1: Shunt Current: 16.40 mA

I (35385) INA3221: C2: Bus Voltage: 11.98 V
I (35385) INA3221: C2: Shunt Voltage: 1.68 mV
I (35395) INA3221: C2: Shunt Current: 16.80 mA

I (35395) INA3221: C3: Bus Voltage: 11.98 V
I (35395) INA3221: C3: Shunt Voltage: 1.60 mV
I (35405) INA3221: C3: Shunt Current: 16.00 mA

I (35415) INA219: Bus Voltage: 12.00 V
I (35415) INA219: Shunt Voltage: 4.61 mV
I (35415) INA219: Current: 0.046100 A
I (35415) INA219: Power: 0.554000 W
I (36425) INA219: Bus Voltage: 12.00 V
I (36425) INA219: Shunt Voltage: 4.61 mV
I (36425) INA219: Current: 0.046100 A
I (36425) INA219: Power: 0.554000 W
I (36905) INA3221: C1: Bus Voltage: 11.98 V
I (36905) INA3221: C1: Shunt Voltage: 1.64 mV
I (36905) INA3221: C1: Shunt Current: 16.40 mA

I (36905) INA3221: C2: Bus Voltage: 11.98 V
I (36905) INA3221: C2: Shunt Voltage: 1.68 mV
I (36915) INA3221: C2: Shunt Current: 16.80 mA

I (36915) INA3221: C3: Bus Voltage: 11.98 V
I (36915) INA3221: C3: Shunt Voltage: 1.60 mV
I (36925) INA3221: C3: Shunt Current: 16.00 mA

I (37425) INA219: Bus Voltage: 12.00 V
I (37425) INA219: Shunt Voltage: 4.62 mV
I (37425) INA219: Current: 0.046200 A
I (37425) INA219: Power: 0.554000 W
"""

# Regular expression to capture sensor data
pattern = re.compile(
    r"I \((\d+)\) (INA\d+|SYSTEM_MONITOR): (.+?)(?=\nI|\Z)", re.DOTALL
)

parsed_data = []

for match in pattern.finditer(log_data):
    timestamp = int(match.group(1))
    source = match.group(2)
    payload = match.group(3).strip()

    # Break into lines and structure based on sensor type
    lines = payload.split("\n")
    for line in lines:
        row = {
            "Timestamp": timestamp,
            "Source": source,
        }

        if source == "INA219":
            if "Bus Voltage" in line:
                row["Metric"] = "Bus Voltage"
                row["Value"] = float(re.search(r"([\d.]+) V", line).group(1))
            elif "Shunt Voltage" in line:
                row["Metric"] = "Shunt Voltage"
                row["Value"] = float(re.search(r"([\d.]+) mV", line).group(1))
            elif "Current" in line:
                row["Metric"] = "Current"
                row["Value"] = float(re.search(r"([\d.]+) A", line).group(1))
            elif "Power" in line:
                row["Metric"] = "Power"
                row["Value"] = float(re.search(r"([\d.]+) W", line).group(1))

        elif source == "INA3221":
            ch_match = re.search(r"C(\d):", line)
            if ch_match:
                channel = ch_match.group(1)
                row["Channel"] = f"C{channel}"
                if "Bus Voltage" in line:
                    row["Metric"] = "Bus Voltage"
                    row["Value"] = float(re.search(r"([\d.]+) V", line).group(1))
                elif "Shunt Voltage" in line:
                    row["Metric"] = "Shunt Voltage"
                    row["Value"] = float(re.search(r"([\d.]+) mV", line).group(1))
                elif "Shunt Current" in line:
                    row["Metric"] = "Current"
                    row["Value"] = float(re.search(r"([\d.]+) mA", line).group(1))

        elif source == "SYSTEM_MONITOR":
            row["Metric"] = "Free heap"
            row["Value"] = int(re.search(r"(\d+) bytes", line).group(1))

        parsed_data.append(row)

# Convert to DataFrame
df = pd.DataFrame(parsed_data)

# Save to Excel
df.to_excel("sensor_log_data.xlsx", index=False)

print("Excel file created successfully: sensor_log_data.xlsx")
