#!/bin/bash

#参数个数为1(第一个参数为程序的名字)，则默认是nrndn_test
if [ $# != 1 ];then
	program_name="nrndn_test";
else
	program_name="nrndn_20160126_easonOriginal";
fi

#定义时间变量
git_date=$(date)
shell_date=$(date +%Y%m%d-%H%M%S)
vehicle_num=$(cat  ~/input/routes.rou.xml | grep "</vehicle>" | wc -l)
file_name=$shell_date"-"$vehicle_num"nodes_"$program_name

#输出shell_data
echo $file_name

mkdir ~/$file_name

cd ~/ndn/ns-3/
#先进行编译
./waf --run "$program_name --method=3"

#后台运行ndn
./waf --run "$program_name --method=0" > ~/$file_name/ndn_record.txt &
#后台运行dis
sleep 4
./waf --run "$program_name --method=1" > ~/$file_name/dis_record.txt &
#后台运行cds
sleep 4
./waf --run "$program_name --method=2" > ~/$file_name/cds_record.txt &

#等待后台程序结束
wait

#把结果合并输出到result.txt
cat ~/input/NR-NDN-Simulation/result.txt ~/input/Dist-Simulation/result.txt  ~/input/CDS-Simulation/result.txt > ~/$file_name/result.txt

#追加，统一显示实验结果
echo "//NDN DIST CDS" >> ~/$file_name/result.txt
tail -2 ~/input/NR-NDN-Simulation/result.txt >> ~/$file_name/result.txt
tail -1 ~/input/Dist-Simulation/result.txt   >> ~/$file_name/result.txt
tail -1 ~/input/CDS-Simulation/result.txt    >> ~/$file_name/result.txt

cp ~/$file_name/result.txt ~/$file_name/README.md

#先去更新ndc-ns3-result文件夹
cd ~/ndc-ns3-result
git pull

#把文件夹复制到github文件夹中
mv ~/$file_name ~/ndc-ns3-result/$file_name

cd ~/ndc-ns3-result
#git add $file_name/ndn_record.txt
#git add $file_name/dis_record.txt
#git add $file_name/cds_record.txt
git add $file_name/result.txt
#git add $file_name/README.md

git commit -m "$git_date"
git push

#输出开始和结束时间
echo "开始时间："$git_date
end_date=$(date)
echo "结束时间："$end_date 

tail -4 ~/ndc-ns3-result/$file_name/result.txt
exit
