use PicoGUI;
RegisterApp(-side=>left,-width=>100);
open LSF,"ls|";
$s = join ('',<LSF>);
close LSF;
NewWidget(-type=>label,-side=>all,-text=>NewString($s));
EventLoop;
