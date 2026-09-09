#!/usr/bin/env python3
import sys
import json
import urllib.request
import os
import subprocess
import time
from urllib.error import URLError, HTTPError

CONTEXT_FILE = "/tmp/ss_ai_context.json"
MODEL_CONFIG_FILE = "/tmp/ss_ai_model.json"
BASE_DIR = os.path.dirname(os.path.abspath(__file__))
OLLAMA_URL = os.environ.get("SS_OLLAMA_URL", "http://localhost:11434/api/chat")
OLLAMA_TAGS_URL = os.environ.get("SS_OLLAMA_TAGS_URL", "http://localhost:11434/api/tags")
OLLAMA_PULL_URL = os.environ.get("SS_OLLAMA_PULL_URL", "http://localhost:11434/api/pull")
LLAMA_BIN = os.environ.get("SS_LLAMA_BIN", os.path.join(BASE_DIR, "third_party/llama.cpp/build/bin/llama-cli"))
LLAMA_MODEL_FILE = os.environ.get("SS_LLAMA_MODEL_FILE", os.path.join(BASE_DIR, "models/qwen-0.5b-coder.gguf"))

SYSTEM_PROMPT = (
    "Tu és o SSBot, o assistente e co-pilot inteligente oficial integrado no SSCodeIDE. "
    "És um profissional do ramo da linguística e gramática da língua portuguesa. "
    "Sempre que responderes em português (qualquer que seja a variante do utilizador), DEVES cumprir rigorosamente as seguintes regras:\n"
    "1. Responde exclusivamente em Português de Angola (pt-AO) seguindo a norma culta e tratando o utilizador por 'tu'.\n"
    "2. Aplica estritamente as regras gramaticais do pré-acordo ortográfico (anterior a 2012), em que as consoantes mudas prevalecem (ex.: acção, directo, objecto, projecto, eléctrico, contacto, óptimo, exacto, correcção, etc.).\n"
    "3. Realiza com absoluta correcção e precisão as conjugações proclítica, mesoclítica e enclítica dos pronomes clíticos.\n"
    "4. Auxilia o utilizador a programar, debugar, explicar código, sugerir comandos e navegar no projecto com elevado rigor técnico."
)

def get_active_model():
    if os.path.exists(MODEL_CONFIG_FILE):
        try:
            with open(MODEL_CONFIG_FILE, "r", encoding="utf-8") as f:
                cfg = json.load(f)
                if "model" in cfg and cfg["model"]:
                    return cfg["model"]
        except Exception:
            pass
    return os.environ.get("SS_OLLAMA_MODEL", "SSBot")

def set_active_model(model_name):
    try:
        with open(MODEL_CONFIG_FILE, "w", encoding="utf-8") as f:
            json.dump({"model": model_name}, f)
        return True
    except Exception:
        return False

def list_ollama_models():
    models = []
    try:
        req = urllib.request.Request(OLLAMA_TAGS_URL)
        with urllib.request.urlopen(req, timeout=4) as resp:
            data = json.loads(resp.read().decode("utf-8"))
            models = [m.get("name", "") for m in data.get("models", [])]
    except Exception:
        try:
            res = subprocess.run(["ollama", "list"], capture_output=True, text=True, timeout=5)
            if res.returncode == 0:
                lines = res.stdout.strip().split("\n")
                if len(lines) > 1:
                    models = [line.split()[0] for line in lines[1:] if line.strip()]
        except Exception:
            pass
    return models

def pull_ollama_model(model_name):
    print(f"// Starting download of model '{model_name}' via Ollama...")
    try:
        proc = subprocess.run(["ollama", "pull", model_name], text=True, check=False)
        if proc.returncode == 0:
            set_active_model(model_name)
            return f"// Model '{model_name}' downloaded successfully and set as active."
    except FileNotFoundError:
        pass
    except Exception as e:
        print(f"// Warning from Ollama CLI: {e}")

    try:
        payload = {"name": model_name, "stream": False}
        req = urllib.request.Request(
            OLLAMA_PULL_URL,
            data=json.dumps(payload).encode("utf-8"),
            headers={"Content-Type": "application/json"}
        )
        with urllib.request.urlopen(req, timeout=600) as resp:
            set_active_model(model_name)
            return f"// Model '{model_name}' downloaded successfully via Ollama API and set as active."
    except Exception as e:
        return f"// Error downloading model '{model_name}': {e}. Make sure Ollama is running."

def ensure_ollama_model():
    """Garante que um modelo válido existe no Ollama."""
    active_model = get_active_model()
    models = list_ollama_models()

    if active_model in models:
        return True
    for m in models:
        if m.startswith(active_model):
            return True

    if active_model == "SSBot" and os.path.isfile(LLAMA_MODEL_FILE):
        modelfile_content = (
            f"FROM {LLAMA_MODEL_FILE}\n"
            "PARAMETER temperature 0.3\n"
            f"SYSTEM \"{SYSTEM_PROMPT.replace(chr(10), ' ')}\"\n"
        )
        modelfile_path = "/tmp/SSBot_Modelfile"
        try:
            with open(modelfile_path, "w", encoding="utf-8") as f:
                f.write(modelfile_content)
            subprocess.run(["ollama", "create", "SSBot", "-f", modelfile_path], capture_output=True, timeout=60)
            if os.path.exists(modelfile_path):
                os.remove(modelfile_path)
            return True
        except Exception:
            pass

    if models:
        set_active_model(models[0])
        return True

    return False

def load_context():
    if os.path.exists(CONTEXT_FILE):
        try:
            with open(CONTEXT_FILE, "r", encoding="utf-8") as f:
                history = json.load(f)
                if history and history[0].get("role") == "system":
                    history[0]["content"] = SYSTEM_PROMPT
                return history
        except:
            pass
    return [{
        "role": "system", 
        "content": SYSTEM_PROMPT
    }]

def save_context(history):
    try:
        with open(CONTEXT_FILE, "w", encoding="utf-8") as f:
            json.dump(history, f, ensure_ascii=False, indent=2)
    except OSError:
        pass

def extract_llama_cli_reply(raw_output):
    if not raw_output:
        return ""

    lines = raw_output.replace("\r", "").split("\n")
    start_idx = -1
    end_idx = len(lines)

    for i, line in enumerate(lines):
        if line.startswith("> "):
            start_idx = i + 1
            break
    if start_idx == -1:
        return raw_output.strip()

    for i in range(start_idx, len(lines)):
        line = lines[i].strip()
        if line.startswith("[ Prompt:") or line == "Exiting...":
            end_idx = i
            break

    reply_lines = lines[start_idx:end_idx]
    while reply_lines and not reply_lines[0].strip():
        reply_lines.pop(0)
    while reply_lines and not reply_lines[-1].strip():
        reply_lines.pop()
    return "\n".join(reply_lines).strip()

def generate_with_ollama(history):
    active_model = get_active_model()
    payload = {
        "model": active_model,
        "messages": history,
        "stream": False,
        "options": {
            "temperature": 0.3,
            "num_predict": 256
        }
    }

    req = urllib.request.Request(
        OLLAMA_URL,
        data=json.dumps(payload).encode("utf-8"),
        headers={"Content-Type": "application/json"}
    )
    with urllib.request.urlopen(req, timeout=120) as response:
        res_data = json.loads(response.read().decode("utf-8"))
        return res_data["message"]["content"].strip()

def generate_with_llama_cpp(history):
    if not os.path.isfile(LLAMA_BIN):
        raise FileNotFoundError("llama-cli not found at: " + LLAMA_BIN)
    if not os.path.isfile(LLAMA_MODEL_FILE):
        raise FileNotFoundError("GGUF model not found at: " + LLAMA_MODEL_FILE)

    system_prompt = ""
    user_prompt = ""

    for item in history:
        role = item.get("role", "")
        if role == "system" and not system_prompt:
            system_prompt = item.get("content", "")
        if role == "user":
            user_prompt = item.get("content", "")

    cmd = [
        LLAMA_BIN,
        "-m", LLAMA_MODEL_FILE,
        "-cnv",
        "-st",
        "-sys", system_prompt,
        "-p", user_prompt,
        "-n", "256",
        "--temp", "0.3",
        "-c", "512",
        "--no-display-prompt",
        "--no-perf",
        "--simple-io",
        "--log-disable"
    ]
    proc = subprocess.run(cmd, capture_output=True, text=True, check=False)
    output = (proc.stdout or "") + ("\n" + proc.stderr if proc.stderr else "")
    output = output.strip()
    reply = extract_llama_cli_reply(output)

    if not reply:
        if not output:
            output = "no output from model"
        raise RuntimeError("llama.cpp failed: " + output)
    return reply

def generate_response(prompt):
    if prompt == "__CLEAR_CONTEXT__":
        if os.path.exists(CONTEXT_FILE):
            os.remove(CONTEXT_FILE)
        return ""

    history = load_context()
    history.append({"role": "user", "content": prompt})
    assistant_message = ""

    ensure_ollama_model()

    try:
        assistant_message = generate_with_ollama(history)
    except (URLError, HTTPError, TimeoutError, json.JSONDecodeError, KeyError, ValueError) as ollama_error:
        try:
            assistant_message = generate_with_llama_cpp(history)
        except (OSError, RuntimeError, FileNotFoundError) as llama_error:
            return (
                "// AI backend error.\n"
                f"// Ollama: {str(ollama_error)}\n"
                f"// llama.cpp: {str(llama_error)}\n"
                "// Hint: You can download models in Ollama by running /pull <model> in SSCodeIDE."
            )

    history.append({"role": "assistant", "content": assistant_message})
    save_context(history)
    return assistant_message.strip()

def interactive_cli():
    print("\033[1;36m" + "=" * 58 + "\033[0m")
    print("\033[1;33m       SSBot CLI - Intelligent Co-Pilot for SSCodeIDE\033[0m")
    print("\033[1;36m" + "=" * 58 + "\033[0m")
    print("Interactive CLI mode active.")
    print("Commands: \033[1;32m/help\033[0m, \033[1;32m/models\033[0m, \033[1;32m/pull <model>\033[0m, \033[1;32m/clear\033[0m, \033[1;31m/quit\033[0m\n")

    while True:
        try:
            user_input = input("\033[1;32mSSBot ❯ \033[0m").strip()
        except (EOFError, KeyboardInterrupt):
            print("\nShutting down SSBot CLI...")
            break
        if not user_input:
            continue
        if user_input in ("/quit", "/exit", "exit"):
            print("Goodbye!")
            break
        if user_input == "/help":
            print("\033[1;33mSSBot CLI Commands:\033[0m")
            print("  /help           - Display this help message")
            print("  /models         - List available Ollama models")
            print("  /pull <model>   - Download a model via Ollama")
            print("  /clear          - Clear conversation history")
            print("  /model          - Show active model")
            print("  /quit           - Terminate CLI session")
            print("  <text>          - Send prompt to SSBot\n")
            continue
        if user_input == "/models":
            models = list_ollama_models()
            active = get_active_model()
            print(f"Active model: \033[1;32m{active}\033[0m")
            print("Available models:")
            for m in models:
                prefix = " * " if m == active else "   "
                print(f"{prefix}{m}")
            print()
            continue
        if user_input.startswith("/pull "):
            target_model = user_input[6:].strip()
            if target_model:
                res = pull_ollama_model(target_model)
                print(res + "\n")
            else:
                print("Usage: /pull <model_name>\n")
            continue
        if user_input == "/model":
            print(f"Configured model: {get_active_model()}\n")
            continue
        if user_input == "/clear":
            generate_response("__CLEAR_CONTEXT__")
            print("\033[1;32mConversation history cleared successfully.\033[0m\n")
            continue

        print("\033[2m[SSBot processing...]\033[0m")
        reply = generate_response(user_input)
        print(f"\n\033[1;34m󱚥 SSBot:\033[0m\n{reply}\n")

RECOMMENDED_MODELS = [
    "qwen2.5-coder:0.5b",
    "qwen2.5-coder:1.5b",
    "qwen2.5-coder:7b",
    "llama3.2:1b",
    "llama3.2:3b",
    "codellama:7b",
    "mistral:7b",
    "phi3:mini",
    "gemma2:2b",
    "deepseek-r1:1.5b",
    "deepseek-coder:1.3b"
]

def list_all_models():
    installed = list_ollama_models()
    active = get_active_model()

    print(f"ACTIVE_MODEL:{active}")
    seen = set()

    for m in installed:
        if m not in seen:
            seen.add(m)
            status = "[INSTALLED]"
            if m == active or m.startswith(active):
                status = "[ACTIVE]"
            print(f"{status} {m}")

    for m in RECOMMENDED_MODELS:
        if m not in seen:
            seen.add(m)
            print(f"[AVAILABLE] {m}")

if __name__ == "__main__":
    if len(sys.argv) > 1:
        arg1 = sys.argv[1]
        if arg1 == "--list" or arg1 == "--list-all":
            list_all_models()
        elif arg1 == "--pull" and len(sys.argv) > 2:
            target_m = sys.argv[2]
            print(pull_ollama_model(target_m))
        elif arg1 == "--set-model" and len(sys.argv) > 2:
            target_m = sys.argv[2]
            if set_active_model(target_m):
                print(f"// Active model changed to '{target_m}'.")
            else:
                print(f"// Error changing active model to '{target_m}'.")
        else:
            user_prompt = arg1
            print(generate_response(user_prompt))
    else:
        interactive_cli()

