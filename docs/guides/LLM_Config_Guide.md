# 🔑 LLM API 配置指南

## 📍 配置文件位置

**文件路径:**
```
d:\CombatDemos\AINPC\Config\DefaultGame.ini
```

---

## 📝 当前配置

```ini
[LLM.Settings]
ApiKey=sk-2468a8422712496ea19806ce760702ad
ApiUrl=https://api.deepseek.com/chat/completions
ModelName=deepseek-chat
```

---

## 🔧 配置说明

### 1. ApiKey (必需)
**作用:** 你的 DeepSeek API 密钥  
**格式:** `sk-xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx`  
**获取方式:** 
- 访问 [DeepSeek 官网](https://platform.deepseek.com/)
- 登录账号
- 在 API Keys 页面创建新密钥

**当前值:** `sk-2468a8422712496ea19806ce760702ad`

### 2. ApiUrl (可选)
**作用:** API 端点地址  
**默认值:** `https://api.deepseek.com/chat/completions`  
**说明:** 如果留空，代码会自动使用默认值

**支持的API:**
- DeepSeek: `https://api.deepseek.com/chat/completions`
- OpenAI: `https://api.openai.com/v1/chat/completions`
- 其他兼容OpenAI格式的API

### 3. ModelName (未使用)
**作用:** 模型名称  
**说明:** 当前代码中硬编码为 `deepseek-chat`，这个配置项暂时无效

---

## 🔄 如何修改配置

### 方法1: 直接编辑文件

1. 打开文件:
   ```
   d:\CombatDemos\AINPC\Config\DefaultGame.ini
   ```

2. 修改配置:
   ```ini
   [LLM.Settings]
   ApiKey=你的新密钥
   ApiUrl=https://api.deepseek.com/chat/completions
   ```

3. 保存文件

4. **重启编辑器** (配置在启动时读取)

---

### 方法2: 在UE编辑器中修改 (推荐)

1. 打开 UE 编辑器
2. 菜单: `Edit` → `Project Settings`
3. 搜索 `Game` 或找到 `Project - Description`
4. 滚动到底部，点击 `Open DefaultGame.ini`
5. 编辑并保存
6. 重启编辑器

---

## 🔍 代码读取逻辑

配置在 `CognitionComponent::BeginPlay()` 中读取：

```cpp
// CognitionComponent.cpp Line 24-25
GConfig->GetString(TEXT("LLM.Settings"), TEXT("ApiKey"), ConfigApiKey, GGameIni);
GConfig->GetString(TEXT("LLM.Settings"), TEXT("ApiUrl"), ConfigApiUrl, GGameIni);
```

**读取时机:** AI Controller 的 `BeginPlay()` 执行时

---

## ⚠️ 常见问题

### 问题1: "API Key not found in DefaultGame.ini!"

**原因:** 配置文件中没有 `[LLM.Settings]` 部分或 `ApiKey` 为空

**解决:**
```ini
[LLM.Settings]
ApiKey=sk-你的密钥
ApiUrl=https://api.deepseek.com/chat/completions
```

---

### 问题2: API调用失败 401 Unauthorized

**原因:** API Key 无效或过期

**解决:**
1. 检查 API Key 是否正确
2. 访问 DeepSeek 控制台验证密钥状态
3. 如果过期，创建新密钥并更新配置

---

### 问题3: API调用失败 Network Error

**原因:** 网络连接问题或 API URL 错误

**解决:**
1. 检查网络连接
2. 验证 `ApiUrl` 是否正确
3. 尝试在浏览器中访问 API 端点

---

### 问题4: 修改配置后不生效

**原因:** 配置在启动时读取，运行时修改不会生效

**解决:**
1. 保存 `DefaultGame.ini`
2. **关闭并重启 UE 编辑器**
3. 重新运行游戏

---

## 🔐 安全建议

### ⚠️ 不要提交 API Key 到 Git

**方法1: 使用 .gitignore**
```gitignore
# .gitignore
Config/DefaultGame.ini
```

**方法2: 使用环境变量 (高级)**
修改代码从环境变量读取：
```cpp
FString ConfigApiKey = FPlatformMisc::GetEnvironmentVariable(TEXT("DEEPSEEK_API_KEY"));
```

---

## 📊 配置验证

### 启动时日志

**成功:**
```
LogTemp: [Cognition] Brain Initialized via Config.
```

**失败:**
```
LogTemp: Error: [Cognition] FATAL: API Key not found in DefaultGame.ini!
```

### 测试API连接

运行游戏后，触发一个感知事件（例如靠近AI），查看日志：

**成功:**
```
[LLM] Request Sent (ID: 0x12345678): Saw Player
[LLM] Processing response for request 0x12345678
[LLM] Success! Parsed: Anger=0.10, Fear=0.20
```

**失败:**
```
[LLM] API Error Code: 401. Response: {"error": "Invalid API Key"}
```

---

## 🌐 支持的LLM服务商

### 1. DeepSeek (当前使用)
```ini
ApiUrl=https://api.deepseek.com/chat/completions
ModelName=deepseek-chat
```

### 2. OpenAI
```ini
ApiUrl=https://api.openai.com/v1/chat/completions
ModelName=gpt-4
```
**注意:** 需要修改代码中的 `model` 字段

### 3. Azure OpenAI
```ini
ApiUrl=https://your-resource.openai.azure.com/openai/deployments/your-deployment/chat/completions?api-version=2023-05-15
```

### 4. 本地 Ollama (免费)
```ini
ApiUrl=http://localhost:11434/v1/chat/completions
ModelName=llama2
```
**注意:** 需要先安装并运行 Ollama

---

## 🔄 切换到其他LLM服务

### 步骤1: 修改配置文件
```ini
[LLM.Settings]
ApiKey=新服务的API密钥
ApiUrl=新服务的API端点
```

### 步骤2: 修改代码 (如果需要)

如果新服务使用不同的模型名称，修改 `LLMCommunicator.cpp`:

```cpp
// Line 39
RootObject->SetStringField("model", "gpt-4");  // 改为新模型
```

### 步骤3: 测试
重启编辑器并测试API连接

---

## 📝 完整配置示例

### DeepSeek (推荐)
```ini
[LLM.Settings]
ApiKey=sk-2468a8422712496ea19806ce760702ad
ApiUrl=https://api.deepseek.com/chat/completions
ModelName=deepseek-chat
```

### OpenAI GPT-4
```ini
[LLM.Settings]
ApiKey=sk-proj-xxxxxxxxxxxxxxxxxxxxx
ApiUrl=https://api.openai.com/v1/chat/completions
ModelName=gpt-4
```

### 本地 Ollama (免费)
```ini
[LLM.Settings]
ApiKey=ollama  # 本地服务不需要真实密钥，但不能为空
ApiUrl=http://localhost:11434/v1/chat/completions
ModelName=llama2
```

---

## 🎯 快速检查清单

配置完成后，检查以下内容：

- [ ] `DefaultGame.ini` 中有 `[LLM.Settings]` 部分
- [ ] `ApiKey` 不为空且格式正确
- [ ] `ApiUrl` 指向正确的API端点
- [ ] 已保存文件
- [ ] 已重启 UE 编辑器
- [ ] 启动时看到 "Brain Initialized via Config" 日志
- [ ] 测试感知事件能触发LLM请求

---

## 📞 需要帮助？

如果配置有问题：

1. **检查日志** - 查看 `LogTemp` 中的错误信息
2. **验证API Key** - 在 DeepSeek 控制台测试
3. **测试网络** - 确保能访问 API 端点
4. **查看文档** - 参考 DeepSeek API 文档

---

**配置文件位置:** `d:\CombatDemos\AINPC\Config\DefaultGame.ini`  
**当前API服务:** DeepSeek  
**当前模型:** deepseek-chat  
**配置状态:** ✅ 已配置
