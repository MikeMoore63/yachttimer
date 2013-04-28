//
// Stop watch timer originally based upon stop watch atimer code from Katherine Berry so many thanks to Katherine
// for a starting set of code. Now rewritten with [proper model.
//
// Aim of this is to have stop watch plus a countdown timer.
//
// Aim of countdown timer is to provide timier facilities for yacht racing
// Normally 5 mins for first gun, 4 minute gun 1 minute and then start.
// read more at link below.
//
// http://en.wikipedia.org/wiki/Racing_Rules_of_Sailing#Start_signal
//
// In countdown mode lap timer targets next rounded timer down or up. So if at anything below 5:00
// would reset to 4:00
//
// Anything below 4:00 and above 2:00 would be 4:00 if below 2:00 would be 1:00.
// Reset takes back to 5:00.
//
// Vibrates at 4 minutes and 1 minute and final timer with custome vibrate pattern. 
// Times of reset are recorde as lap timers (Mainly as helps debug and for feedback as I use it).
//
// long press on select button toggles mode and aim is to toggle while running between modes
// as well as when stopped. Lap times/resets don't change.
// Will work on timer/chrono flag as well.
//
// Now has adjustable countdown mode as well.
//
// Added watch mode so you can get to time while countdown/stopwatch is running and switch back
//
// Also added ability to flip face colour
//
// Released as is at your own risk caveat emptor. Enjoy - Mike
//
