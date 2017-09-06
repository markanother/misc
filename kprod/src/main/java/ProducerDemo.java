public class ProducerDemo {
    public static void main(String[] args) {
        boolean isAsync = args.length == 0 || !args[0].trim().equalsIgnoreCase("sync");
        Producer producerThread = new Producer(KafkaProperties.TOPIC, isAsync);
        producerThread.start();
        //System.out.println("www");
        try {
            Thread.sleep(1000);
            producerThread.join();
        } catch (Exception e) {
            e.printStackTrace();
        }
    }
}
