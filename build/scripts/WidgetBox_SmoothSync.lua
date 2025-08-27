local lastMin, lastMax, boxmin, boxmax, a, useClassicInterpolation = ...

displayMin = vec2(0);
displayMax = vec2(0);

-- a = a + 1

displayMin = mix(lastMin, boxmin, smoothstep(0, 1, a));
displayMax = mix(lastMax, boxmax, smoothstep(0, 1, a));

-- if useClassicInterpolation == 1 then
--     -- displayMin = mix(lastMin, boxmin, smoothstep(0, 1, a));
--     -- displayMax = mix(lastMax, boxmax, smoothstep(0, 1, a));
--     displayMin = boxmin;
--     displayMax = boxmax;
-- else
--     displayMin = mix(boxmax, boxmin, smoothstep(0, 1, a));
--     displayMax = mix(boxmin, boxmax, smoothstep(0, 1, a));
-- end
