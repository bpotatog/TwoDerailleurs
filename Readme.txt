* communication between android app and arduino
- via bluetooth
- message format
	-- 4 bytes per message
	-- e.g.     f  @  @  1
	--          -  -  -  -
	--          ∆  first byte: header for [front derailleur position]
	--             ∆  second byte: filled by meaningless note
	--                ∆  third byte: filled by meaningless note
	--                   ∆  forth byte: [front derailleur position]
	--         front position at 1st gear

	-- e.g.     r  @  1  0
	--          -  -  -  -
	--          ∆  first byte: header for [rear derailleur position]
	--             ∆  second byte: filled by meaningless note
	--                ∆  third byte: [rear derailleur position tens digit]
	--                   ∆  forth byte: [rear derailleur position ones digit]
	--         rear position at 10th gear

	-- e.g.     P  X  X  X
	--          -  -  -  -
	--          ∆  first byte: header for [pedal rate info]
	--             from 2nd till 4th digit is pedal rate(cadence), if pedal rate is under 100, 
	--             filled by @ note.
	--         P@87 = pedal rate 87 RPM
	-- e.g.     T  X  X  X
	--          -  -  -  -
	--          ∆  first byte: header for [bicycle speed info]
	--             from 2nd till 4th digits is bicycle speed, if bicycle speed is under 100, 
	--             filled by @ note.
	--         T@30 = bicycle speed 30 KM/hour

