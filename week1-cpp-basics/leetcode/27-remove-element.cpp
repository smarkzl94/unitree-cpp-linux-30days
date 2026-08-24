// LeetCode 27. Remove Element（Easy）
// 给你数组 nums 和值 val，原地删掉所有等于 val 的元素，返回新长度 k。
// 前 k 个可以是任意顺序。O(1) 额外空间。
//
// 和 Day03 vector 的关系：不要真的 erase 中间导致后面全搬；用双指针覆盖即可。
// 和 erase-remove 是同一思想：把「要留的」挤到前面。

#include <iostream>
#include <vector>

class Solution {
public:
    int removeElement(std::vector<int>& nums, int val) {
        int k = 0;  // 下一个「留下的元素」该写到哪
        for (int x : nums) {
            if (x != val) {
                nums[k] = x;
                ++k;
            }
        }
        return k;
    }
};

int main() {
    std::vector<int> nums{3, 2, 2, 3};
    int k = Solution().removeElement(nums, 3);
    // 期望 k==2，前两个都是 2
    std::cout << "k=" << k << " front:";
    for (int i = 0; i < k; ++i) std::cout << " " << nums[i];
    std::cout << "\n";
    return k == 2 ? 0 : 1;
}
