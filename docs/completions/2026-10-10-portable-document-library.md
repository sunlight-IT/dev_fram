# 2026-10-10 - 可迁移框架文档库

## 实际范围与来源

按已审查方案建立子模块文档库；仅 bus/port/BSP、DMA、ring、公共定义与 TIM/PWM。
框架基线 `d29419be990aa562bcbacabb17ac9c78b31a0908`；主工程导入基线
`8ab7a95033b30b5848def22971a35bacab766645`；开始时两工作区干净。

来源为固定 Git 对象、公开头文件、原规则及历史记录。原文件/blob hashes、行范围、
脱敏/摘录转换和输出 hashes 由来源清单、文档元数据和 ZIP 校验清单分别保存。

## 变更

新增 AI 入口、现行规则、当前状态/API、宿主接入、历史路径表、两类规范草案与 43 条核查表、
历史计划/设计/日志/完成摘录、证据索引、文档检查和导出工具。源码快照只引用公开头文件于文档，
没有打包 `.c/.h` 文件或修改固件。主工程只增加导航、本次记录和生成索引。

## 验证命令与观察

开发前执行 `python Usr/hal/tests/docs_library_test.py`，4 项失败，原因均为文档工具尚不存在。
随后增加入口正文篡改负向测试，先实际观察到原检查错误通过，再加入完整入口 hash 校验。
当前已实际执行并通过的命令：

```powershell
python Usr/hal/scripts/import-kg200z-history.py --host-root . --check
python Usr/hal/scripts/docs-library.py update
python Usr/hal/scripts/docs-library.py check
python Usr/hal/tests/docs_library_test.py
powershell -NoProfile -ExecutionPolicy Bypass -File Usr/hal/scripts/docs-index.ps1 -Check
git diff --check
git -C Usr/hal diff --check
```

- 来源复核：`[IMPORT] Verified 40 document provenance records from 67 pinned sources`。
- 文档库：`[DOCS] PASS: 40 files, 67 sources, provenance, excerpt hashes, links and index`。
- 验收测试：6 项、退出码 0；覆盖正文/文件修改、未登记文件、无效来源、断链、
  历史摘录不可被 update 重新认证、ZIP 篡改、无 Git 解压迁移。
- PowerShell 文档入口：parser 无错误，实际 `-Check` 通过。
- 主工程和子模块 tracked diff-check：退出码 0。
- 源码保持检查：39 个 `.c/.h` 的 Git-normalized blob 与固定基线一致；固件 diff 为空。
- 43 条草案规则核查表已建立；没有把静态表述升级为运行合规结论。

## 资料包

导出命令是 `python Usr/hal/scripts/docs-library.py export`；命名使用来源日期和框架源码基线。
包内有 42 个库文件及 `SHA256SUMS`，外部有 `.zip.sha256`，共 43 个 ZIP entries。
工具使用 Python 3.10+ 标准库，PowerShell wrapper 需要 Python 在 PATH。

已实际执行并通过：

```powershell
python Usr/hal/scripts/docs-library.py export
Expand-Archive -LiteralPath Usr/hal/exports/dev-fram-docs-2026-10-10-d29419b.zip -DestinationPath 'Usr/hal/exports/relocated project'
python 'Usr/hal/exports/relocated project/scripts/docs-library.py' check
python 'Usr/hal/exports/relocated project/tests/docs_library_test.py'
Get-FileHash -LiteralPath Usr/hal/exports/dev-fram-docs-2026-10-10-d29419b.zip -Algorithm SHA256
```

- 导出输出：`[DOCS] PASS: archive inventory and all checksums`，42 个库文件。
- 实际包完整解压后：40 个文件/67 个来源检查通过，6 项工具测试通过，退出码 0。
- 逐项扫描 ZIP：无 `.git`、固件 `.c/.h`、ELF/HEX/BIN、SDK 或本机配置；sidecar 与 ZIP hash 一致。
- 本记录写入后重新导出，最终 hash 位于外部 `.zip.sha256`，避免把 ZIP 自身 hash 写入包内形成循环。
- 历史导入：18 份摘录、44 个来源片段；31 份 docs Markdown；40 条文件来源记录，67 个固定源文件。
- Python 3 个工具/测试文件 AST 解析和 PowerShell wrapper parser 检查通过。

## 未验证事项与风险

- 未执行固件 build/flash，未连接硬件；旧构建/烧录/运行记录没有升级为当前验证。
- 跨工程规范仍是 Draft，本次没有批准全部规范或修复源码合规差距。
- 历史记录精确被测 commit/image 不足时维持“版本未确定”。
- root 旧规范路径检查原先失败；本次保留原规范内容，并单独报告其状态。
- 没有可调用的独立 reviewer subagent；本次使用主代理自审和可执行文档工具检查。
- 文档传播需要维护者提交并发布子模块、更新宿主 gitlink；本次不执行 commit/push。

## 关联

- [实施计划](../plans/2026-10-10-portable-document-library.md)
- [证据索引](evidence-index.md)
- [文档导航](../README.md)
