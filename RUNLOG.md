# RUNLOG

| Profile | delay_ms | Miss % | Overhead | What I changed and why |
|---|---|---|---|---|
| A | 40 | 5.73% | 1.02x | Built and ran the naive baseline. It fails because dropped packets are never recovered, and the lack of a jitter buffer means delayed packets are automatically marked as missed. |
| B | 40 | 70.33% | 1.02x | Ran baseline on Profile B to see the stress test. Massive failure. The 5% loss rate and 80ms jitter completely breaks the naive approach. I need to implement a mechanism to recover dropped packets. |
| A | 150 | - | - | **Idea:** Decided against ARQ (retransmissions) because the round-trip time would kill the delay score. Instead, I tried basic Forward Error Correction (FEC) by sending `seq`, `payload[i]`, and `payload[i-1]` in every packet. **Result:** Realized this would result in a `324 / 160 = 2.025x` overhead, instantly failing the bandwidth cap before even running it. |
| B | 150 | 0.81% | 1.94x | **Fixing Overhead:** To get under the 2.0x cap, I modified the sender to skip sending the redundant `payload[i-1]` on every 12th frame. I set a generous delay of 150ms to ensure it worked. **Result:** It passed! The overhead dropped to exactly 1.94x and the miss rate stayed safely below 1%. |
| B | 100 | 0.67% | 1.94x | **Shrinking Delay:** 150ms is too high. I know Profile B has an 80ms max jitter. Since my FEC recovers the dropped packet when the *next* packet arrives (adding 20ms of intrinsic delay), the theoretical minimum delay should be `80 + 20 = 100ms`. I tested 100ms and it safely passed with 10 misses out of 1500 frames. |
| B | 95 | 1.20% | 1.94x | **Stress Testing:** Tried to squeeze the delay down to 95ms just to see if I could get away with it. The miss rate climbed to 1.20%, invalidating the run. This confirms my math: packets delayed by the network to the 80ms limit, plus the 20ms recovery time, miss the 95ms deadline. |
| B | 100 | 0.67% | 1.94x | **Locked it in.** Reverted to 100ms as the absolute lowest mathematically safe delay for Profile B; final validation stayed under both caps. |
