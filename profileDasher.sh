# profileDasher <Base input RTSP URI> <Max dashers> <Config File> <timeout seconds>
# <Max dashers> testdash will be instantiated simultaneously.

date >> profileDasher.log
echo $0 $1 $2 $3 >> profileDasher.log

date >> profileDasher.stats
echo $0 $1 $2 $3 >> profileDasher.stats
echo "Config file	Num Dashers	avgDelay	Block Losses	IN bitra	Pkt Losses	uCPU%	sCPU%	tCPU%" >> profileDasher.stats

for n in `seq 1 1 $2`
do
    commandline="./profiledash -r $1 -f /tmp -timeout $4 -statsfile profileDasher.stats -configfile $3 -n $n"
    echo $commandline
    echo $commandline >> profileDasher.log
    $commandline >> profileDasher.log 2>&1
done
