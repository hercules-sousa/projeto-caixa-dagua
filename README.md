# Relatório do projeto

## Instituto Federal de Educação, Ciência e Tecnologia da Paraíba

## Curso de Bacharelado em Engenharia de Computação

## Disciplina: Sistemas Embarcados

## Equipe: Matheus Alves da Silva e Hércules de Sousa Silva

---

### Introdução

O objetivo deste projeto é a criação de um sistema de monitoramento e controle de temperatura através do uso de sensores e atuadores com a finalidade de pôr em prática os conhecimentos obtidos ao longo da disciplina de sistemas embarcados e de construir o protótipo de um produto que pode ser útil nos mais diversos cenários, desde sistemas de irrigação até processos industriais.

A aplicação a ser desenvolvida será composta por um microcontrolador, que nada mais é do que um circuito integrado que contém um processador, memória e periféricos de entrada e saída em um único chip, o modelo que será utilizado é o ESP-32, além disso esse sistema irá contar com um sensor de temperatura e um sensor de nível de água, que permitirá monitorar o grau de calor e de água do reservatório para que através de atuadores possa-se realizar o controle dessas variáveis. Os atuadores que serão utilizados vão uma bomba para ajustar o nível de água e uma resistência para promover o aquecimento do sistema. E por fim, para servir de interface entre o usuário e o hardware será utilizado um display LCD.

### Objetivos

Este projeto tem como objetivos:

- Realizar o monitoramento de temperatura e nível de água em tempo real;
- Garantir que tanto o nível quanto a temperatura do reservatório estejam sempre acima de um ponto determinado;
- Prover uma interface para a configuração das variáveis monitoradas;
- Diminuir o disperdício de água;

### Casos de uso

**Cenário 1:**

**Descrição:** o sistema deve mostrar o nível de água e temperatura do reservatório realizando medições a cada segundo.

**Atores:** ESP-32, sensor de temperatura, sensor de nível e o display LCD.

**Pré-condições:** nenhuma.

**Pós-condições:** o registro passa a ser exibido no display LCD.

**Requisitos funcionais:**

- Criar uma estrutura de dados para temperatura, nível de água, data e hora;
- O sistema deve conseguir exibir no display a temperatura e nível de água.

**Requisitos não funcionais:**

- Confiabilidade: os dados mostrados precisam estar sempre disponíveis;
- Verossimilhança: as informações exibidas precisam corresponder às medições feitas.

**Cenário 2:**

**Descrição:** a aplicação deve monitorar a temperatura do reservatório e ligar a resistência para aquecê-la caso a quantidade de calor no sistema desça abaixo de um grau indesejado.

**Atores:** ESP-32, sensor de temperatura e resistência.

**Pré-condições:** a temperatura está abaixo do ponto desejado

**Pós-condições:** a temperatura do sistema deve voltar a um grau aceitável.

**Requisitos funcionais:**

- Acionar a resistência para aquecer a água, caso a temperatura fique abaixo do grau definido;
- Guardar o parâmetro que irá indicar o nível mínimo de temperatura da água.

**Requisitos não funcionais:**

- Confiabilidade: o sistema deve funcionar de forma consistente e sem falhas, em outras palavras, a aplicação deve sempre garantir que a temperatura esteja acima do que foi previamente estabelecido;

**Cenário 3:**

**Descrição:** a aplicação deve monitorar o nível do reservatório e ligar a bomba para enchê-la caso a quantidade de água no sistema desça abaixo do mínimo desejado.

**Atores:** ESP-32, sensor de nível e bomba de água.

**Pré-condições:** o nível de água está abaixo do ponto desejado.

**Pós-condições:** o nível de água do sistema deve voltar a um grau aceitável.

**Requisitos funcionais:**

- Acionar a bomba de água, caso o nível fique abaixo do grau definido;
- Guardar em uma variável o nível que irá indicar a altura mínima do reservatório.

**Requisitos não funcionais:**

- Confiabilidade: o sistema deve funcionar de forma consistente e sem falhas, isto é, ele deve sempre garantir que a água esteja acima da altura definida.

**Cenário 4:**

**Descrição:** a aplicação deve permitir a inserção de valores de entrada para atribuir qual o nível mínimo de nível de água e temperatura para que esses dados sejam utilizados para a ativação do atuadores.

**Atores:** ESP-32 e um periférico de entrada.

**Pré-condições:** nenhuma.

**Pós-condições:** os dados são salvos e utilizados como parâmetros para a ativação dos atuadores.

**Requisitos funcionais:**

- Possibilitar ao usuário inserir dados sobre a temperatura mínima do sistema;
- Possibilitar ao usuário inserir dados sobre o nível de água mínimo do sistema;
- Guardar as informações em alguma estrutura de dados.

**Requisitos não funcionais:**

- Usabilidade: o usuário não deve demorar mais de 1 min para descobrir como inserir os dados na aplicação.
