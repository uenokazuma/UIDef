#include "Visualization.h"

void loadModel() {
    if (model == NULL) {

        TF_Status status = TF_NewSession(TF_NewSessionOptions(), &model);
    }
}