# Git Commit Message 规范

本文档定义《动手学音视频开发》项目的Git提交信息规范，确保提交历史清晰、可读。

---

## 📋 提交信息格式

```
<type>(<scope>): <subject>

<body>

<footer>
```

### 1. 标题行（必填）
```
<type>(<scope>): <subject>
```

**type（类型）：**
- `feat`：新功能
- `fix`：修复bug
- `docs`：文档更新
- `style`：代码格式调整（不影响功能）
- `refactor`：代码重构（不改变功能）
- `perf`：性能优化
- `test`：测试相关
- `chore`：构建过程或辅助工具的变动
- `ci`：持续集成相关
- `build`：构建系统或外部依赖变更

**scope（范围）：** 可选，说明提交影响的范围
- `lesson`：课程内容
- `notebook`：Jupyter笔记本
- `docs`：文档
- `assets`：测试素材
- `build`：构建配置
- `ci`：持续集成
- `deps`：依赖管理

**subject（主题）：** 简短描述，不超过50字符
- 使用祈使语气（如"添加"、"修复"、"更新"）
- 首字母小写
- 不加句号

### 2. 正文（可选）
- 详细说明提交的内容
- 解释为什么需要这个变更
- 描述具体做了什么
- 每行不超过72字符

### 3. 页脚（可选）
- 关联的Issue编号：`Closes #123`
- 破坏性变更说明：`BREAKING CHANGE:`
- 其他相关信息

---

## 📝 提交信息示例

### 新课程开发
```
feat(lesson): 添加第2节 - 视频的色彩空间

- 实现RGB到YUV420P转换
- 添加YUV分量分离实验
- 包含4个课后挑战任务

Closes #2
```

### 文档更新
```
docs: 更新课程开发指南

- 添加内存管理最佳实践
- 完善颜色空间转换说明
- 更新进度追踪表
```

### 代码修复
```
fix(notebook): 修复图像显示颜色问题

- 修正OpenCV到matplotlib的颜色转换
- 更新代码注释说明BGR/RGB差异

Fixes #15
```

### 测试素材添加
```
chore(assets): 添加测试视频素材

- 下载5秒360p测试视频
- 添加音频测试文件
- 更新素材下载脚本
```

### 依赖更新
```
build(deps): 更新Python依赖版本

- opencv-python: 4.8.0 → 4.9.0
- numpy: 1.24.0 → 1.25.0
- 更新requirements.txt
```

---

## 🎯 课程开发相关提交规范

### 新课程开发
```
feat(lesson): 添加第X节 - [课程标题]

- [核心实验1描述]
- [核心实验2描述]
- 包含[数量]个课后挑战任务

Closes #[Issue编号]
```

### 课程更新
```
docs(lesson): 更新第X节讲义

- 添加理论讲解内容
- 完善实验步骤说明
- 更新课后挑战题目
```

### Jupyter笔记本
```
feat(notebook): 添加第X节课实验代码

- 实现[实验名称]实验
- 添加[数量]个代码单元
- 包含数据可视化图表
```

### 测试素材
```
chore(assets): 准备第X节课测试素材

- 添加[视频/音频/图像]素材
- 更新素材下载脚本
- 验证素材规格符合要求
```

---

## 🔧 Git工作流规范

### 1. 分支命名
- `main`：主分支，稳定版本
- `develop`：开发分支
- `feature/lesson-[编号]`：新课程开发
- `fix/[问题描述]`：问题修复
- `docs/[文档类型]`：文档更新

### 2. 提交频率
- 每节课开发完成后提交一次
- 每个独立功能/修复单独提交
- 避免一次性提交大量变更

### 3. 提交前检查
- 运行代码测试（如有）
- 检查代码格式
- 验证提交信息格式
- 确保无敏感信息泄露

### 4. 合并请求
- 每个功能分支合并前创建PR
- PR标题格式：`[类型] 描述`
- PR描述包含变更摘要
- 关联相关Issue

---

## 📊 当前提交历史规范化

### 现有提交分析

| 提交哈希 | 原始提交信息 | 规范化建议 |
|----------|--------------|------------|
| b1df855 | `feat: 添加第1节课 - 数字图像基础` | ✅ 符合规范 |
| 488dcde | `docs: 完善课程蓝图和开发文档` | ✅ 符合规范 |

### 建议的规范化提交示例

**第2节课开发：**
```
feat(lesson): 添加第2节 - 视频的色彩空间

- 实现RGB到YUV420P转换公式
- 添加YUV分量分离实验
- 实现U/V分量涂黑效果
- 包含3个课后挑战任务

Closes #2
```

**第3节课素材准备：**
```
chore(assets): 准备第3节课音频素材

- 下载5秒PCM测试音频
- 添加44.1kHz和48kHz样本
- 更新音频处理脚本
```

**构建配置更新：**
```
build: 添加C++项目CMake配置

- 添加根CMakeLists.txt
- 配置ASAN内存检测
- 添加模块目录结构
```

---

## 🚀 快速参考

### 常用提交类型
- `feat(lesson):` 新课程开发
- `docs:` 文档更新
- `fix(notebook):` 实验代码修复
- `chore(assets):` 测试素材管理
- `build:` 构建配置

### 提交信息检查清单
- [ ] 类型前缀正确（feat/fix/docs等）
- [ ] 范围明确（lesson/notebook/docs等）
- [ ] 主题简短明确（<50字符）
- [ ] 使用祈使语气
- [ ] 正文详细说明（如有需要）
- [ ] 关联Issue（如有）

### 工具支持
```bash
# 查看提交历史
git log --oneline --graph --all

# 查看具体提交
git show <commit-hash>

# 修改最后一次提交
git commit --amend

# 交互式rebase
git rebase -i HEAD~3
```

---

## 📚 参考资源

- [Conventional Commits](https://www.conventionalcommits.org/)
- [Angular Commit Message Guidelines](https://github.com/angular/angular/blob/main/CONTRIBUTING.md#commit)
- [Git Commit Best Practices](https://cbea.ms/git-commit/)

---

**文档版本：** v1.0
**最后更新：** 2026-04-24
**维护者：** opencode
