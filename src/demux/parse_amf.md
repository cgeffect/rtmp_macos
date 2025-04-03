详解视频封装格式之FLV

https://blog.csdn.net/weekend_y45/article/details/125400311?spm=1001.2101.3001.6650.1&utm_medium=distribute.pc_relevant.none-task-blog-2%7Edefault%7EBlogCommendFromBaidu%7ERate-1-125400311-blog-136872202.235%5Ev43%5Econtrol&depth_1-utm_source=distribute.pc_relevant.none-task-blog-2%7Edefault%7EBlogCommendFromBaidu%7ERate-1-125400311-blog-136872202.235%5Ev43%5Econtrol&utm_relevant_index=2


音视频入门基础：FLV专题（7）——Tag header简介
https://blog.csdn.net/u014552102/article/details/142600413

FLV 格式解析
https://xie.infoq.cn/article/ee5b8c4fdab1017abfe12f8e3

---

根据你提供的 FLV 文件中的 Script Tag 数据，我们可以按照文章中的解析方法来提取音视频的元数据信息。以下是逐步解析的过程：

### 1. **Script Tag 数据结构**
根据文章中的描述，Script Tag 的数据结构如下：

在 AMF0 格式中，每个数据类型都有其特定的结构和长度。
02 00 0A 6F 6E 4D 65 74 61 44 61 74 61  08 00 00 00 19 

字符串解析
02 表示字符串
00 0A 取两位表示长度, 表示数据的长度 10
6F 6E 4D 65 74 61 44 61 74 61 表示实际的数据

数组解析
08 表示EMCA
00 00 00 19 取四位表示长度, 表示长度为25

如果类型是02, 表示数据类型是字符串, 数据长度为10, 直接往后找找10个

如果类型是08, 表示数据类型是数组, 00 00 00 19表示的数组的长度, 而不是数组数据长度, 然后每一个数组的key和value又有规则去获取
解析的规则为:
2字节表示key值的长度,    00 08：表示 PropertyName 的长度为 8 字节。 
N字节表示 key的值,      64 75 72 61 74 69 6f 6e：表示 PropertyName 的内容为 duration。
1字节表示value类型,     00：表示 PropertyData 的类型为双精度浮点数（AMF0 类型）。
固定8字节表示value的值,  40 41 14 39 58 10 62 4e：表示 PropertyData 的值，这是一个 IEEE-754 双精度浮点数。
一个键值对的结束符,      00：表示该键值对结束。

不同的数据类型解析的规则不同