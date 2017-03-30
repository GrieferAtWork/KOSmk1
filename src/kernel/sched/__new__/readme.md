This was originally meant to be the implementation of a new scheduler.

But as anyone who has ever dared implementing one can confirm (such as me: "../task*"), these things are not that easy, and (at least for me) complicated enough to make being put aside for only a couple of days before returning to finish the job later impossible without forgetting its inner workings.

Doing so might work for many other aspects (even within something as intriquing as a kernel), but not a scheduler.

Locking on such a low level just gets too complicated and relies on too many small aspects to resolve all its race conditions, which isn't something you can document fast enough before starting to forget parts of its layout, causing you to start doubting the integrity of the entire plan (Especially when it comes to the multi-dimensional complexity of getting global, per/inter-cpu and per/inter-thread comunication synchronized while also never forgetting the fact that threads constantly change).

Anyways... Just as with the first scheduler I originally wrote for KOS (../__old__), I will keep this implementation around, as it might proove the next time I'm feeling lucky and got 10-14 hours to kill.

But until then, I'll either focus on smaller things or try and design a new scheduler in much smaller steps.
