# UbiRemote: Sistema de controle remoto inteligente

### Acesse nosso [Hackster.Io](https://www.hackster.io/anabxalves/ubiremote-46a233) 🔗

## Equipe
<table align="center">
	<tr>
		<td align="center">
			<a href="https://github.com/anabxalves">
				<img src="https://avatars.githubusercontent.com/u/108446826?v=4" width="200px;" alt="Foto Ana"/><br>
				<sub>
					<b>Ana Beatriz Alves</b>
				</sub>
			</a>
		</td>
		<td align="center">
			<a href="https://github.com/Caiobadv">
				<img src="https://avatars.githubusercontent.com/u/117755420?v=4" width="200px;" alt="Foto Caio"/><br>
				<sub>
					<b>Caio Barreto</b>
				</sub>
			</a>
		</td>
		<td align="center">
			<a href="https://github.com/VictorHTenorio">
				<img src="https://avatars.githubusercontent.com/u/101901740?v=4" width="200px;" alt="Foto Victor"/><br>
				<sub>
					<b>Victor Hora</b>
				</sub>
			</a>
		</td>
	</tr>
</table>

### 1. Visão geral do projeto

O UbiRemote é um sistema de controle remoto inteligente que permite o gerenciamento remoto de vários dispositivos por meio de uma interface da Web moderna. O sistema usa sinais de infravermelho para se comunicar com os dispositivos existentes, o que o torna uma solução não invasiva que funciona com a maioria dos dispositivos sem exigir modificações de hardware.

O projeto oferece uma solução econômica para o gerenciamento de vários dispositivos sem a necessidade de dispositivos inteligentes caros. Ao aproveitar a infraestrutura existente e adicionar controle inteligente, ele oferece uma abordagem prática para o gerenciamento moderno de dispositivos.

#### 1.1. Principais recursos
- Gerenciamento de vários dispositivos: Controle várias unidades de ar condicionado a partir de uma única interface
- Monitoramento de status em tempo real: Rastreie o estado de cada unidade (ocupado/vago)
- Aprendizado de sinais de infravermelho: Captura e armazena códigos IR para diferentes modelos de dispositivo
- Controle remoto: Opere os dispositivos de qualquer lugar por meio de uma interface da Web
- UI moderna: Interface de usuário limpa e intuitiva criada com Next.js e Tailwind CSS
- Atualizações em tempo real: Feedback instantâneo sobre o status do dispositivo e ações de controle

### 2. Implementação técnica

#### 2.1. Hardware
- Microcontrolador ESP32
- Receptor e transmissor de infravermelho
- Conectividade WiFi

#### 2.2. Software
- Frontend: Next.js 14 com TypeScript
- Componentes da interface do usuário: Tailwind CSS e Shadcn UI
- Backend: Banco de dados em tempo real do Firebase
- Firmware: PlatformIO com estrutura Arduino
- Controle de IR: Biblioteca IRremoteESP8266

### 3. Como funciona

O ESP32 captura sinais de IR de controles remotos CA existentes e os sinais são armazenados no Firebase com informações específicas do dispositivo. Por meio da interface da Web, os usuários podem:
- Registrar novos dispositivos
- Ver o status do dispositivo
- Enviar comandos para unidades específicas
- Monitorar a operação em tempo real

### 4. Casos de uso
- Prédios de escritórios: Gerenciar vários dispositivos em diferentes cômodos
- Casas inteligentes: Integrar o controle de AC aos sistemas de automação residencial
- Instituições educacionais: Monitorar e controlar os dispositivos de aula (TV, Som, Projetor)

### 5. Aprimoramentos futuros
- Integração com plataformas de casa inteligente
- Monitoramento do consumo de energia
- Programação de temperatura
- Desenvolvimento de aplicativos móveis
- Integração de controle de voz

### 6. Primeiros passos

#### 6.1. Pré-requisitos
- Node.js e npm instalados
- IDE PlatformIO (extensão VS Code)
- Conta do Firebase
- Placa de desenvolvimento ESP32
- Módulos de receptor e transmissor de IR

#### 6.2. Etapa 1: Configuração do Firebase
- Crie um novo projeto do Firebase em [https://console.firebase.google.com/](https://console.firebase.google.com/)
- Habilite o banco de dados em tempo real
- Vá para Configurações do projeto > Contas de serviço
- Clique em "Generate New Private Key" (Gerar nova chave privada) para fazer download do JSON de configuração do Firebase
- Crie um arquivo .env.local no diretório ventila-ai com os seguintes parâmetros:

```
    NEXT_PUBLIC_FIREBASE_API_KEY=your_api_key
    NEXT_PUBLIC_FIREBASE_AUTH_DOMAIN=your_auth_domain
    NEXT_PUBLIC_FIREBASE_DATABASE_URL=your_database_url
    NEXT_PUBLIC_FIREBASE_PROJECT_ID=your_project_id
    NEXT_PUBLIC_FIREBASE_STORAGE_BUCKET=your_storage_bucket
    NEXT_PUBLIC_FIREBASE_MESSAGING_SENDER_ID=your_messaging_sender_id
    NEXT_PUBLIC_FIREBASE_APP_ID=your_app_id
```

#### 6.3. Etapa 2: Configuração do ESP32
- Abra o projeto tela3 no VS Code com a extensão PlatformIO
- Conecte o ESP32 ao computador
- Configure suas credenciais de WiFi em src/main.cpp :

```
    #define WIFI_SSID "your_wifi_ssid"
    #define WIFI_PASSWORD "your_wifi_password"
```

- Atualizar as credenciais do Firebase em src/main.cpp :

```
    #define DATABASE_URL "your_firebase_database_url"
    #define DATABASE_SECRET "your_firebase_database_secret"
```

- Crie e carregue o firmware:

    a. Clique no botão "Build" (✓) do PlatformIO\
    b. Clique no botão "Upload" (→)\
    c. Aguarde a conclusão do upload

#### 6.4. Etapa 3: Configuração do front-end\
- Navegue até o diretório ventila-ai\
- Instale as dependências:

```
    npm install
```

- Inicie o servidor de desenvolvimento:

```
    npm run dev
```

- Abra [http://localhost:3000](http://localhost:3000) em seu navegador

#### 6.5. Etapa 4: Primeiro uso
- O ESP32 se conectará automaticamente ao WiFi e ao Firebase
- Abra a interface da Web
- Adicione seu primeiro dispositivo clicando no botão "Add Device" (Adicionar dispositivo)
- Siga as instruções na tela para capturar sinais IR

#### 6.6. Solução de problemas
- Se o ESP32 não conseguir se conectar, verifique suas credenciais de WiFi
- Se a conexão com o Firebase falhar, verifique o URL e o segredo do banco de dados
- Se a interface da Web mostrar erros de conexão, verifique o arquivo .env.local
- Para problemas de captura de infravermelho, verifique se o receptor de infravermelho está conectado corretamente ao pino 4
- Para problemas de transmissão de infravermelho, verifique se o LED de infravermelho está conectado ao pino 2

#### 6.7. Conexões de hardware
- Receptor de infravermelho: Conectado ao GPIO 4
- LED DE INFRAVERMELHO: Conectar ao GPIO 2
- Alimentação: Conecte o ESP32 à fonte de alimentação de 5V

#### 6.8. Observações
- O ESP32 se reconectará automaticamente se a conexão for perdida
- Os sinais de IR são armazenados no Firebase para armazenamento persistente
- A interface da Web é atualizada em tempo real à medida que os dispositivos são controlados.