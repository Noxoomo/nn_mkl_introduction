from python.experiments import data_loading
import sys
import os
import numpy as np

sys.path.append("../../../cmake-build-debug/cpp/experiments/cifar_nn")
import cifar_nn_py


def main():
    path = os.path.join(os.path.curdir, '../../resources', 'cifar10')
    train_images, train_labels, test_images, test_labels = data_loading.cifar10(path)
    train_images = train_images.reshape((50000, 3, 32, 32))
    test_images = test_images.reshape((10000, 3, 32, 32))

    ds = cifar_nn_py.Dataset(train_images, train_labels)
    print(ds)
    net = cifar_nn_py.SimpleNet()
    print(net)

    cifar_nn_py.train(net, ds, 10)

    res = net.forward(test_images)
    print(res.shape)
    res = np.argmax(res, axis=1)

    diff = test_labels - res
    sim = np.where(diff == 0, 1, 0)
    accuracy = np.sum(sim)/sim.shape[0]

    print(accuracy)


if __name__ == "__main__":
    main()