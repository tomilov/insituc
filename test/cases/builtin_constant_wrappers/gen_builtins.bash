symbols=( zero one pi l2e l2t lg2 ln2 )
for symbol in ${symbols[@]}
do
    echo -e "function ${symbol}_() return ${symbol} end" | tee "${symbol}.txt"
done
