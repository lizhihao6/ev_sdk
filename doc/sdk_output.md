# 极市算法SDK输出协议


[toc]

## 1. 概要

此文档是极视角算法SDK输出的制定协议，所有算法SDK的输出应按照这一协议定义输出。

| 更新时间  | 版本 |    备注     |
| :-------: | :--: | :---------: |
| 2021-02-1 | 1.0  | 发布1.0版本 |

协议主要基于几个考量因素制定标准：

- 满足业务需求；

- 能够进行有效的算法精度测试；

- 输出数据清晰易懂；

- 新算法能够依据此协议制定合理的输出格式。

## 2. 格式标准

### 2.1 基础规范

数据格式必须遵守[JSON标准](http://json.cn/wiki.html)。

#### 2.1.1 命名规范

- 命名采用蛇形命名法：所有命名使用小写字母和下划线表示；

#### 2.1.2 数值规范

- 所有键含义为**是/否**的数值必须使用`true`、`false`，不能使用0、1等数字表示；
- 所有与置信度相关的数值类型必须是`float`，且保留小数点后三位；

#### 2.1.3 结构规范

- 最外层为对象类型；
- 最外层对象**必须存在**的键：`model_data`、`algorithm_data`，且对应值类型必须为对象类型，其中：
  - `algorithm_data`表示业务输出，是对原始模型输出进行过滤、修改后满足业务需求的数据；
  - `model_data`表示模型输出的原始数据，用于评估原始算法模型的精度。

### 2.2 常用数据结构规范

#### 2.2.1 使用矩形框表示的目标位置

- 坐标使用以图像横向为x轴、纵向为y轴、左上顶点为原点的直角坐标系表示法；

- 使用左上顶点坐标、矩形框宽、矩形框高表示单个目标框，格式规范：

  ```json
  {
      "x": "Number（int）：坐标值",
      "y": "Number（int）：坐标值",
      "width": "Number（int）：矩形框宽",
      "height": "Number（int）：矩形框高"
  }
  ```

- 矩形框左上顶点坐标、宽、高分别使用`(x, y)`、`width`、`height`表示。

#### 2.2.2 跟踪ID

对于带有跟踪信息的算法，必须将跟踪ID与目标框一起输出，且其键名称为`object_id`，值为整型。

### 2.3 业务输出数据规范

#### 2.3.1 基础规范

业务输出相关的数据必须包含在`algorithm_data`键下。

- 对于报警类算法，必须存在键`is_alert`、`target_info`；
- 对于非报警类算法，必须存在键`target_info`。

#### 2.3.2 报警状态

- `is_alert`：布尔类型，可选，表示当前数据是否是报警状态，其中`true`表示报警，`false`表示不报警；

#### 2.3.3 详细业务信息

业务信息必须使用键`target_info`存储。

- `target_info`，存储业务数据，要求：
  - 当信息为空时，值为`null`；
  - 当信息不为空时，为算法输出的具体业务信息，可以为对象类型和数组类型，对于不同的算法类别，具有不同的规范，参考以下针对每种类别算法的详细定义。
- `target_count`：整型，可选，当`target_info`值为数组类型时，表示数组的长度；
  - 如果存在多个目标数量，必须以`target_count`作为前缀命名，如`target_count_clothes`，`target_count_no_clothes`；

- 其他业务所需字段按照2.1、2.2、2.3规范进行添加。

#### 2.3.4 目标检测类算法

##### 2.3.4.1 目标检测+单标签分类

**格式规范**

```json
{
    "algorithm_data": {
        "is_alert": true,
        "target_count": 1,
        "target_info": [
            {
                "x": "Number（int）：坐标值",
                "y": "Number（int）：坐标值",
                "width": "Number（int）：矩形框宽",
                "height": "Number（int）：矩形框高",
                "name": "String：类别名称",
                "confidence": "Number（float）：当前目标的置信度"
            }
        ]
    }
}
```

**详细说明**

  - `target_info`的值类型为列表，列表可以包括$1-n$个元素；
  - 每个元素必须包含：矩形目标框、目标类别；
  - 如果包含置信度，键名为`confidence`；
  - 目标类别名称使用`name`表示，且其值必须可以唯一区分当前目标所属的类别。

##### 2.3.4.2 目标检测+多标签分类

**格式标准**

```json
{
    "algorithm_data": {
        "is_alert": true,
        "target_count": 1,
        "target_info": [
            {
                "x": "Number（int）：坐标值",
                "y": "Number（int）：坐标值",
                "width": "Number（int）：矩形框宽",
                "height": "Number（int）：矩形框高",
                "classes": [
                    {
                        "class": "String: 类别名称",
                        "confidence": "Number（float）: 该类别的置信度"
                    }
                ]
            }
        ]
    }
}
```

**详细说明**

- `target_info`的值类型为列表，列表可以包括$1-n$个元素；
- `target_info`内的每个元素包含：矩形目标框、目标类别；
- 目标类别使用`classes`表示，且其值类型必须是列表，且列表内可以包含$1-n$个元素，$n$表示所有类别数。
- 如果包含置信度，键名为`confidence`；
- 目标的类别名称使用`name`表示，且这一名称必须可以唯一区分当前目标所属的类别。

#### 2.3.5 分类算法

##### 2.3.5.1 单标签分类

**格式规范**

```json
{
    "algorithm_data": {
        "is_alert": true,
        "target_info": {
            "class": "String: 类别名称",
            "confidence": "Number（float）: 类别的置信度"
        }
    }
}
```

**详细说明**

- `target_info`的值类型为对象；
- 目标类别使用`class`表示，且其值类型为字符串；
- 如果包含置信度，键名为`confidence`；

##### 2.3.5.2 多标签分类

**格式规范**

```json
{
    "algorithm_data": {
        "is_alert": true,
        "target_info": [
            {
                "class": "String: 类别名称",
                "confidence": "Number（float）: 该类别的置信度"
            }
        ]
    }
}
```

**详细说明**

- `target_info`的值类型为数组，每个元素存储一个类别信息，数组内可以包含$1-n$个元素，$n$表示所有类别数；
- 如果包含置信度，键名为`confidence`。

### 2.4 模型输出数据规范

#### 2.4.1 支持的模型类别

- 目标检测类算法
  - 目标检测+单标签，类别名称：`object_detection_single_label`；
  - 目标检测+多标签，类别名称：`object_detection_multi_label`；
- 分类算法
  - 单标签分类，类别名称：`classification_single_label`；
  - 多标签分类，类别名称：`classification_multi_label`；

#### 2.4.2 基础规范

- 模型输出数据必须放在`model_data`键下，对于不同的算法类别，具有不同的规范，参考以下针对每种类别算法的详细定义；

- 对于只使用了单个模型的算法，模型输出数据存放在`model_data`的值字段；

  ```json
  {
      "model_data": "对象或数组：单个模型的原始输出"
  }
  ```

- 对于使用了多个模型的算法，`model_data`的值必须使用如下数据对象结构：

  ```json
  {
      "model_data": [
          {
              "model_index": "Number（int）：模型索引",
              "model_type": "String：模型类别",
              "value": "对象或数组：单个模型的原始输出"
          }
      ]
  }
  ```

  其中：

  - `model_index`：整型，表示当前模型输出字段`value`所属的模型索引，必须唯一；
  - `model_type`：字符串，表示当前模型的类别，可选值参考2.4.1；
  - `value`：表示当前模型的原始输出内容，对于不同的算法类别具有不同的格式规范，参考下文对不同算法类别的具体要求。

#### 2.4.3 目标检测类算法

##### 2.4.3.1 目标检测+单标签分类

**格式规范**

```json
{
    "objects": [
        {
            "x": "Number（int）：坐标值",
            "y": "Number（int）：坐标值",
            "width": "Number（int）：矩形框宽",
            "height": "Number（int）：矩形框高",
            "name": "String：类别名称",
            "confidence": "Number（float）：当前目标的置信度"
        }
    ]
}
```

**详细说明**

  - `objects`的值类型为列表，列表可以包括$0-n$个元素，当`objects`长度为0时，值为`[]`；
  - 每个元素必须包含：矩形目标框、目标类别；
  - 如果包含置信度，键名为`confidence`；
  - 目标类别名称使用`name`表示，且其值必须可以唯一区分当前目标所属的类别。

##### 2.4.3.2 目标检测+多标签分类

**格式标准**

```json
{
    "objects": [
        {
            "x": "Number（int）：坐标值",
            "y": "Number（int）：坐标值",
            "width": "Number（int）：矩形框宽",
            "height": "Number（int）：矩形框高",
            "classes": [
                {
                    "class": "String: 类别名称",
                    "confidence": "Number（float）: 该类别的置信度"
                }
            ]
        }
    ]
}
```

**详细说明**

- `objects`的值类型为列表，列表可以包括$0-n$个元素，当`objects`长度为0时，值为`[]`；
- `objects`内的每个元素包含：矩形目标框、目标类别；
- 目标类别使用`classes`表示，且其值类型必须是列表，且列表内可以包含$1-n$个元素，$n$表示所有类别数。
- 如果包含置信度，键名为`confidence`；
- 目标的类别名称使用`name`表示，且这一名称必须可以唯一区分当前目标所属的类别。

##### 2.4.3.3 目标检测+跟踪

此类算法需要在2.4.3.1和2.4.3.2的基础上，对所有`object`加上跟踪ID，以目标跟踪+单标签分类为例：

```json
{
    "objects": [
        {
            "x": "Number（int）：坐标值",
            "y": "Number（int）：坐标值",
            "width": "Number（int）：矩形框宽",
            "height": "Number（int）：矩形框高",
            "name": "String：类别名称",
            "object_id": 10,
            "confidence": "Number（float）：当前目标的置信度"
        }
    ]
}
```

#### 2.4.4 分类算法

##### 2.4.4.1 单标签分类

**格式规范**

```json
{
    "class": "String: 类别名称",
    "confidence": "Number（float）: 类别的置信度"
}
```

##### 2.4.4.2 多标签分类

**格式规范**

```json
{
    "classes": [
        {
            "class": "String: 类别名称",
            "confidence": "Number（float）: 该类别的置信度"
        }
    ]
}
```

## 3. 扩展

- 对于业务输出类数据，当2.2、2.3规范无法满足新的模型输出数据类型时（如非目标检测类、非分类算法），需要基于基础规范2.1对2.2、2.4进行扩展；
- 对于模型输出数据，当2.2、2.3规范无法满足新的模型输出数据类型时（如非目标检测类、非分类算法），需要基于基本规范1、2.1规范对2.2、2.3进行扩展；

> 注意：协议扩展应由项目侧收集具体的算法扩展需求，并交由协议制定者对协议进行扩充。如：对于模型输出数据规范，当前仅支分类模型和目标检测类模型，后续若需要支持对语义分割类模型的测试，则需要由协议制定者对此进行扩充。

## 4. 示例
  下面为一些输出示例，特别注意，其中的`null`为json中的空类型，`true`,`false`为json中的布尔型，**非字符串**
### 4.1 安全帽检测（单模型+目标检测+单标签分类）

**报警**

```json
{
    "algorithm_data": {
        "is_alert": true,
        "target_count_hat": 1,
        "target_count_head": 1,
        "target_info": [
            {
                "x": 543,
                "y": 154,
                "width": 37,
                "height": 54,
                "name": "hat_white",
                "confidence": 0.867
            },
            {
                "x": 543,
                "y": 154,
                "width": 37,
                "height": 54,
                "name": "head",
                "confidence": 0.867
            }
        ]
    },
    "model_data": {
        "objects": [
            {
                "x": 543,
                "y": 154,
                "width": 37,
                "height": 54,
                "name": "hat_white",
                "confidence": 0.867
            },
            {
                "x": 543,
                "y": 154,
                "width": 37,
                "height": 54,
                "name": "head",
                "confidence": 0.867
            }
        ]
    }
}
```

**非报警**

```json
{
    "algorithm_data": {
        "is_alert": false,
        "target_count_head": 0,
        "target_count_hat": 0,
        "target_info": null
    },
    "model_data": {
        "objects": []
    }
}
```

```json
{
    "algorithm_data": {
        "is_alert": false,
        "target_count_hat": 1,
        "target_count_head": 0,
        "target_info": [
            {
                "x": 543,
                "y": 154,
                "width": 37,
                "height": 54,
                "name": "hat_white",
                "confidence": 0.867
            }
        ]
    },
    "model_data": {
        "objects": [
            {
                "x": 543,
                "y": 154,
                "width": 37,
                "height": 54,
                "name": "hat_white",
                "confidence": 0.867
            }
        ]
    }
}
```

### 4.2 徘徊检测（单模型+目标检测+单标签分类+跟踪）

**报警**

```json
{
    "algorithm_data": {
        "is_alert": true,
        "target_count": 2,
        "target_info": [
            {
                "height": 56,
                "width": 100,
                "x": 950,
                "y": 666,
                "confidence": 0.568,
                "name": "person",
                "object_id": 1
            },
            {
                "height": 56,
                "width": 100,
                "x": 950,
                "y": 666,
                "confidence": 0.568,
                "name": "person",
                "object_id": 2
            }
        ]
    },
    "model_data": {
        "objects": [
            {
                "height": 56,
                "width": 100,
                "x": 950,
                "y": 666,
                "confidence": 0.568,
                "name": "person",
                "object_id": 1
            },
            {
                "height": 56,
                "width": 100,
                "x": 950,
                "y": 666,
                "confidence": 0.568,
                "name": "person",
                "object_id": 2
            }
        ]
    }
}
```

**非报警**

```json
{
    "algorithm_data": {
        "is_alert": false,
        "target_count": 0,
        "target_info": null
    },
    "model_data": {
        "objects": []
    }
}
```

### 4.3 短袖短裤识别（单模型+目标检测+多标签分类）

**报警**

```json
{
    "algorithm_data": {
        "is_alert": true,
        "target_count": 2,
        "target_info": [
            {
                "x": 320,
                "y": 430,
                "width": 500,
                "height": 700,
                "classes":[
                    {
                        "class": "s_sleeve",
                        "confidence": 0.562
                    },
                    {
                        "class": "shorts",
                        "confidence": 0.882
                    }
                ]
           },
           {
                "x": 320,
                "y": 430,
                "width": 500,
                "height": 700,
                "classes": [
                    {
                        "class": "s_sleeve",
                        "confidence": 0.888
                    },
                    {
                        "class": "unsure",
                        "confidence": 0.561
                    }
                ]
            }
        ]
    },
    "model_data": {
        "objects": [
            {
                "x": 320,
                "y": 430,
                "width": 500,
                "height": 700,
                "classes":[
                    {
                        "class": "s_sleeve",
                        "confidence": 0.562
                    },
                    {
                        "class": "shorts",
                        "confidence": 0.882
                    }
                ]
           },
           {
                "x": 320,
                "y": 430,
                "width": 500,
                "height": 700,
                "classes": [
                    {
                        "class": "s_sleeve",
                        "confidence": 0.888
                    },
                    {
                        "class": "unsure",
                        "confidence": 0.561
                    }
                ]
            }
        ]
    }
}
```

**非报警**

```json
{
    "algorithm_data": {
        "is_alert": true,
        "target_count": 2,
        "target_info": [
            {
                "x": 320,
                "y": 430,
                "width": 500,
                "height": 700,
                "classes":[
                    {
                        "class": "l_sleeve",
                        "confidence": 0.562
                    },
                    {
                        "class": "trousers",
                        "confidence": 0.882
                    }
                ]
           }
        ]
    },
    "model_data": {
        "objects": [
            {
                "x": 320,
                "y": 430,
                "width": 500,
                "height": 700,
                "classes":[
                    {
                        "class": "l_sleeve",
                        "confidence": 0.562
                    },
                    {
                        "class": "trousers",
                        "confidence": 0.882
                    }
                ]
           }
        ]
    }
}
```

### 4.4 农作物识别（单模型+单标签分类）

```json
{
    "algorithm_data": {
        "target_info": [
            {
                "class": "tomato_powdery",
                "confidence": 0.751547
            },
            {
                "class": "tomato_viral",
                "confidence": 0.751547
            },
            {
                "class": "tomato_gray",
                "confidence": 0.751547
            }
        ]
    },
    "model_data": {
        "classes": [
            {
                "class": "tomato_powdery",
                "confidence": 0.751547
            },
            {
                "class": "tomato_viral",
                "confidence": 0.751547
            },
            {
                "class": "tomato_gray",
                "confidence": 0.751547
            }
        ]
    }
}
```

### 4.5 农作物识别（多模型+每个模型都是单标签分类）

```json
{
    "algorithm_data": {
        "target_info": [
            {
                "class": "tomato_powdery",
                "confidence": 0.751547
            },
            {
                "class": "tomato_viral",
                "confidence": 0.751547
            }
        ]
    },
    "model_data": [
        {
            "model_index": 0,
            "model_type": "classification_single_label",
            "value": {
                "class": "tomato_powdery",
                "confidence": 0.751547
            }
        },
        {
            "model_index": 1,
            "model_type": "classification_single_label",
            "value": {
                "class": "tomato_viral",
                "confidence": 0.751547
            }
        }
    ]
}
```
