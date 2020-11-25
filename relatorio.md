# Driver linux para acender

- **Alunos:** André Ejzenmesser e Paulo Tozzo
- **Curso:** Engenharia da Computação
- **Semestre:** 8 / 10
- **Contato:** andree@al.insper.edu.br , paulotp@al.insper.edu.br
- **Ano:** 2020

## Começando

!!! info 
    Esse tutorial parte do principio que o tutorial [Char Device Driver](https://insper.github.io/Embarcados-Avancados/Tutorial-HPS-kernel-chardriver/) já foi feito e concluído.
    
## Requisitos
-Colocar o nome da fita de LED aqui (seria bom colocar aqui exatament que fita a gente esta usando... Não estou chando o nome dela)
-Para usar o driver é necessário compilar o [Buildroot](https://insper.github.io/Embarcados-Avancados/Tutorial-HPS-Buildroot/)

## Objetivo

Criar um driver para um sistema embarcado que possibilite o acesso e controle de uma fita de led.

## Iniciando com uma Função Simples

Partindo do código construído no tutorial [Char Device Driver](https://insper.github.io/Embarcados-Avancados/Tutorial-HPS-kernel-chardriver/) vamos adicionar o acesso a memória para cada vez que acontecer uma ação de escrita no driver.

Para isso vamos inicialmente incluir algumas bibliotecas e definir algumas variáveis globais no começo do código:

```c
// Para tratar o buffer
#include <string.h>

// Definimos o local logico de inicio da fita de led para o hardware
#define LED_PIO_BASE 0X000
// Definimos o tamanho da fita de led
#define LED_PIO_SPAN 256

// Variavel estatica que indicara o local fisico do led
static int *p_led = NULL;
```

Dentro da função de inicialização do driver, vamos adicionar um pedaço de código que vai indicar onde fica o endereço físico de nossa fita, para futuramente, poder escrever o desejado nele.

```c
static int __init ebbchar_init(void){
  //...

  // Funcao responsavel por direcionar o local exato do led no hardware
  p_led = ioremap_nocache(ALT_LWFPGASLVS_OFST + LED_PIO_BASE, LED_PIO_SPAN);

  printk(KERN_INFO "EBBChar: device class created correctly\n"); // Made it! device was initialized
  return 0;
}
```

Por fim, vamos reescrever a função de escrita do driver para poder seguir a lógica do hardware e conseguir escrever na fita de led RGB. Para isso alguns pontos devem ser levados em conta:

- O buffer recebido sempre deve ter 5 itens e devem ser no formato: "Nxrgb"
  
  N - Indicação de inicio de buffer para escrita na fita de led - Valor fixo "N"
  
  x - Posição da fita / Led que será acesso - Valor de 0 até 255
  
  r - Componente vermelho do led - Valor de 0 até 255
  
  g - Componente verde do led - Valor de 0 até 255
  
  b - Componente azul do led - Valor de 0 até 255
  
- Apesar de o padrão de envio para led RGB ser RGB, a fita escolhida tem um comportamento diferente, exigindo que seja enviado no padrão GRB. Por conta disso, deve ser feito um processamento dos dados antes do envio.

!!! info 
    No kernel do linux são várias as funções de disponíveis para manipular hardware/ endereço de memória física. Nos utilizaremos a função:

    ```c
    void iowrite32(u32 value, void __iomem *addr);
    ```

```c
/** @brief This function is called whenever the device is being written to from user space i.e.
*  data is sent to the device from the user. The data is copied to the message[] array in this
*  LKM using the sprintf() function along with the length of the string.
*  @param filep A pointer to a file object
*  @param buffer The buffer to that contains the string to write to the device
* aaaaaaa @param len The length of the array of data that is being passed in the const char buffer
*  @param offset The offset if required
*/
static ssize_t dev_write(struct file *filep, const char *buffer, size_t len, loff_t *offset) {
  // Erro caso o buffer recebido tenha menos que 5 itens
  if (len < 5) {
    printk(KERN_INFO "EBBChar: Received unsuficient characters. Received: %zu characters from the user\n", len);
    return len;
  }

  // Leitura do buffer
  copy_from_user(message, buffer, len);

  // Se nao iniciar com "N" deve dar erro
  if (!strcmp(message[0], "N")) {
    size_of_message = len;                 // store the length of the stored message

    // Processamento das cores
    unsigned int color = 0;

    color |= message[3];
    color <<= 8;
    color |= message[2];
    color <<= 8;
    color |= message[4];

    // Enviar as cores no formato grb para o hardware na posicao desejada
    iowrite32(color, p_led + message[1]);

    printk(KERN_INFO "EBBChar: wrote on led. Received: %zu characters from the user\n", len);
    return len;
  }

  printk(KERN_INFO "EBBChar: wrong init of package. Received: %s\n", message[0]);
  return len;
}
```

Com isso, temos o código simples funcionando, basta agora fazer o build dele em seu hardware e usar como melhor desejar. Abaixo o código completo para ter fácil acesso.

??? example "Código Completo" 
    ```c
    /*
    * @file   ebbchar.c
    * @author Derek Molloy
    * @date   7 April 2015
    * @version 0.1
    * @brief   An introductory character driver to support the second article of my series on
    * Linux loadable kernel module (LKM) development. This module maps to /dev/ebbchar and
    * comes with a helper C program that can be run in Linux user space to communicate with
    * this the LKM.
    * @see http://www.derekmolloy.ie/ for a full description and follow-up descriptions.
    *
    * Corsi 20b: Modifiquei para usar kmalloc no lugar de de alocacao estática.
    *
    */

    #include <linux/init.h>           // Macros used to mark up functions e.g. __init __exit
    #include <linux/module.h>         // Core header for loading LKMs into the kernel
    #include <linux/device.h>         // Header to support the kernel Driver Model
    #include <linux/kernel.h>         // Contains types, macros, functions for the kernel
    #include <linux/fs.h>             // Header for the Linux file system support
    #include <linux/uaccess.h>          // Required for the copy to user function
    #include <linux/slab.h>
    #include <string.h>

    #define  DEVICE_NAME "ebbchar"    ///< The device will appear at /dev/ebbchar using this value
    #define  CLASS_NAME  "ebb"        ///< The device class -- this is a character device driver

    #define LED_PIO_BASE 0X000
    #define LED_PIO_SPAN 256

    MODULE_LICENSE("GPL");            ///< The license type -- this affects available functionality
    MODULE_AUTHOR("Derek Molloy");    ///< The author -- visible when you use modinfo
    MODULE_DESCRIPTION("A simple Linux char driver for the BBB");  ///< The description -- see modinfo
    MODULE_VERSION("0.1");            ///< A version number to inform users

    static int    majorNumber;                  ///< Stores the device number -- determined automatically
    static char   *message;                     ///< Memory for the string that is passed from userspace
    static short  size_of_message;              ///< Used to remember the size of the string stored
    static int    numberOpens = 0;              ///< Counts the number of times the device is opened
    static struct class*  ebbcharClass  = NULL; ///< The device-driver class struct pointer
    static struct device* ebbcharDevice = NULL; ///< The device-driver device struct pointer

    // The prototype functions for the character driver -- must come before the struct definition
    static int     dev_open(struct inode *, struct file *);
    static int     dev_release(struct inode *, struct file *);
    static ssize_t dev_read(struct file *, char *, size_t, loff_t *);
    static ssize_t dev_write(struct file *, const char *, size_t, loff_t *);

    // Variavel estatica indicando o local do led
    static int *p_led = NULL;

    /** @brief Devices are represented as file structure in the kernel. The file_operations structure from
    *  /linux/fs.h lists the callback functions that you wish to associated with your file operations
    *  using a C99 syntax structure. char devices usually implement open, read, write and release calls
    */
    static struct file_operations fops =
    {
      .open = dev_open,
      .read = dev_read,
      .write = dev_write,
      .release = dev_release,
    };

    /** @brief The LKM initialization function
    *  The static keyword restricts the visibility of the function to within this C file. The __init
    *  macro means that for a built-in driver (not a LKM) the function is only used at initialization
    *  time and that it can be discarded and its memory freed up after that point.
    *  @return returns 0 if successful
    */
    static int __init ebbchar_init(void){
      printk(KERN_INFO "EBBChar: Initializing the EBBChar LKM\n");

      // Try to dynamically allocate a major number for the device -- more difficult but worth it
      majorNumber = register_chrdev(0, DEVICE_NAME, &fops);
      if (majorNumber<0){
          printk(KERN_ALERT "EBBChar failed to register a major number\n");
          return majorNumber;
      }
      printk(KERN_INFO "EBBChar: registered correctly with major number %d\n", majorNumber);

      // Register the device class
      ebbcharClass = class_create(THIS_MODULE, CLASS_NAME);
      if (IS_ERR(ebbcharClass)){                // Check for error and clean up if there is
          unregister_chrdev(majorNumber, DEVICE_NAME);
          printk(KERN_ALERT "Failed to register device class\n");
          return PTR_ERR(ebbcharClass);          // Correct way to return an error on a pointer
      }
      printk(KERN_INFO "EBBChar: device class registered correctly\n");

      // Register the device driver
      ebbcharDevice = device_create(ebbcharClass, NULL, MKDEV(majorNumber, 0), NULL, DEVICE_NAME);
      if (IS_ERR(ebbcharDevice)){               // Clean up if there is an error
          class_destroy(ebbcharClass);           // Repeated code but the alternative is goto statements
          unregister_chrdev(majorNumber, DEVICE_NAME);
          printk(KERN_ALERT "Failed to create the device\n");
          return PTR_ERR(ebbcharDevice);
      }

      // alocate mem
      message = kmalloc(32, GFP_KERNEL);
      if (IS_ERR(message)){
          printk(KERN_INFO "Failed to allocate mem \n");
          return PTR_ERR(message);
      }

      p_led = ioremap_nocache(ALT_LWFPGASLVS_OFST + LED_PIO_BASE, LED_PIO_SPAN);

      printk(KERN_INFO "EBBChar: device class created correctly\n"); // Made it! device was initialized
      return 0;
    }

    /** @brief The LKM cleanup function
    *  Similar to the initialization function, it is static. The __exit macro notifies that if this
    *  code is used for a built-in driver (not a LKM) that this function is not required.
    */
    static void __exit ebbchar_exit(void) {
      device_destroy(ebbcharClass, MKDEV(majorNumber, 0));     // remove the device
      class_unregister(ebbcharClass);                          // unregister the device class
      class_destroy(ebbcharClass);                             // remove the device class
      unregister_chrdev(majorNumber, DEVICE_NAME);             // unregister the major number
      kfree(message);                                          // release memmory
      printk(KERN_INFO "EBBChar: Goodbye from the LKM!\n");
    }

    /** @brief The device open function that is called each time the device is opened
    *  This will only increment the numberOpens counter in this case.
    *  @param inodep A pointer to an inode object (defined in linux/fs.h)
    *  @param filep A pointer to a file object (defined in linux/fs.h)
    */
    static int dev_open(struct inode *inodep, struct file *filep) {
      numberOpens++;
      printk(KERN_INFO "EBBChar: Device has been opened %d time(s)\n", numberOpens);
      return 0;
    }

    /** @brief This function is called whenever device is being read from user space i.e. data is
    *  being sent from the device to the user. In this case is uses the copy_to_user() function to
    *  send the buffer string to the user and captures any errors.
    *  @param filep A pointer to a file object (defined in linux/fs.h)
    *  @param buffer The pointer to the buffer to which this function writes the data
    *  @param len The length of the b
    *  @param offset The offset if required
    */
    static ssize_t dev_read(struct file *filep, char *buffer, size_t len, loff_t *offset) {
      int error_count = 0;
      // copy_to_user has the format ( * to, *from, size) and returns 0 on success
      error_count = copy_to_user(buffer, message, size_of_message);

      if (error_count==0){            // if true then have success
          printk(KERN_INFO "EBBChar: Sent %d characters to the user\n", size_of_message);
          return (size_of_message=0);  // clear the position to the start and return 0
      }
      else {
          printk(KERN_INFO "EBBChar: Failed to send %d characters to the user\n", error_count);
          return -EFAULT;              // Failed -- return a bad address message (i.e. -14)
      }
    }

    /** @brief This function is called whenever the device is being written to from user space i.e.
    *  data is sent to the device from the user. The data is copied to the message[] array in this
    *  LKM using the sprintf() function along with the length of the string.
    *  @param filep A pointer to a file object
    *  @param buffer The buffer to that contains the string to write to the device
    * aaaaaaa @param len The length of the array of data that is being passed in the const char buffer
    *  @param offset The offset if required
    */
    static ssize_t dev_write(struct file *filep, const char *buffer, size_t len, loff_t *offset) {
      /**
      * Isso funcionaria??
      *
      *  
      * Receber o buffer no estilo (Nxrgb) -> sendo:
      * N - Char indicador de novo led a ser acessado - valor fixo N mesmo
      * x - led que deve ser acessado - valores de 0 a 255
      * r - red - valores de 0 a 255
      * g - green - valores de 0 a 255
      * b - blue - valores de 0 a 255
      * 
      * Processar os dados recebidos e enviar para a memoria no local certo,
      * ja baseado no local que inicia (LED_PIO_BASE)
      * 
      * enviar para o device[LED_PIO_BASE + x] = rgb -> No formato que o HW
      * espera receber
      * 
      * 
      * Abrir e escrever utilizando - utilizar o que ja temos da entrega 4 adaptado para conseguir fazer isso:
      * virtual_base = mmap( NULL, HW_REGS_SPAN, ( PROT_READ | PROT_WRITE ), MAP_SHARED, fd, HW_REGS_BASE ); -> Colocar isso no init do driver
      * alt_setbits_word( ( virtual_base + ( ( uint32_t )( ALT_GPIO1_SWPORTA_DR_ADDR ) & ( uint32_t )( HW_REGS_MASK ) ) ), BIT_LED );
      * 
      */

      if (len < 5) {
        printk(KERN_INFO "EBBChar: Received unsuficient characters. Received: %zu characters from the user\n", len);
        return len;
      }

      copy_from_user(message, buffer, len);

      if (!strcmp(message[0], "N")) {
        size_of_message = len;                 // store the length of the stored message

        unsigned int color = 0;

        color |= message[3];
        color <<= 8;
        color |= message[2];
        color <<= 8;
        color |= message[4];

        // Enviar a mensagem no formato grb
        iowrite32(color, p_led + message[1]);  // corsi: write to LED

        printk(KERN_INFO "EBBChar: wrote on led. Received: %zu characters from the user\n", len);
        return len;
      }

      printk(KERN_INFO "EBBChar: wrong init of package. Received: %s\n", message[0]);

      return len;
    }

    /** @brieaf The device release function that is called whenever the device is closed/released by
    *  the userspace program
    *  @param inodep A pointer to an inode object (defined in linux/fs.h)
    *  @param filep A pointer to a file object (defined in linux/fs.h)
    */
    static int dev_release(struct inode *inodep, struct file *filep) {
      printk(KERN_INFO "EBBChar: Device successfully closed\n");
      return 0;
    }

    /** @brief A module must use the module_init() module_exit() macros from linux/init.h, which
    *  identify the initialization function at insertion time and the cleanup function (as
    *  listed above)
    */
    module_init(ebbchar_init);
    module_exit(ebbchar_exit);
    ```