exports.Task = extend(TolokaHandlebarsTask, function(options) {
    TolokaHandlebarsTask.call(this, options);
}, {
    setSolution: function(solution) {
        TolokaHandlebarsTask.prototype.setSolution.apply(this, arguments);
    },

    // Обрабатываем сообщение об ошибке.
    addError: function(message, field, errors) {
        errors || (errors = {
            task_id: this.getOptions().task.id,
            errors: {}
        });
        errors.errors[field] = {
            message: message
        };

        return errors;
    },

    validate: function(solution) {
        return TolokaHandlebarsTask.prototype.validate.apply(this, arguments);
    },

    onRender: function() {
        this.rendered = true;
    }
});

function extend(ParentClass, constructorFunction, prototypeHash) {
    constructorFunction = constructorFunction || function() {
    };
    prototypeHash = prototypeHash || {};
    if (ParentClass) {
        constructorFunction.prototype = Object.create(ParentClass.prototype);
    }
    for (var i in prototypeHash) {
        constructorFunction.prototype[i] = prototypeHash[i];
    }
    return constructorFunction;
}
