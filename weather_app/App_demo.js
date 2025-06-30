let ans = 0;

function Salary(a, b, c, d){
    ans = (c + (d / 60)) - (a + (b / 60));
    return ans
}

console.log(Salary(14, 35, 19, 46))