#!/bin/bash
#先去更新ndc-ns3-result文件夹
cd ~/ndc-ns3-result
git pull

#定义时间变量
git_date=$(date)
shell_date=$(date +%Y%m%d-%k%M%S)
file_name="result-"$shell_date

#输出shell_data
echo $file_name

mkdir ~/ndc-ns3-result/$file_name

cd ~/ndn/ns-3/
#先进行编译
./waf --run "nrndn_test --method=3"

#后台运行ndn
./waf --run "nrndn_test --method=0" > ~/ndc-ns3-result/$file_name/ndn_record.txt &
#后台运行dis
sleep 4
./waf --run "nrndn_test --method=1" > ~/ndc-ns3-result/$file_name/dis_record.txt &
#后台运行cds
sleep 4
./waf --run "nrndn_test --method=2" > ~/ndc-ns3-result/$file_name/cds_record.txt &

#等待后台程序结束
wait

#把结果合并输出到result.txt
cat ~/input/NR-NDN-Simulation/result.txt ~/input/CDS-Simulation/result.txt  ~/input/CDS-Simulation/result.txt > ~/ndc-ns3-result/$file_name/result.txt


cd ~/ndc-ns3-result
#git add $file_name/ndn_record.txt
#git add $file_name/dis_record.txt
#git add $file_name/cds_record.txt
git add $file_name/result.txt

git commit -m "$git_date"
git push

#输出开始和结束时间
echo "开始时间："$git_date
end_date=$(date)
echo "结束时间："$end_date 
exit
