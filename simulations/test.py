from DataTypes import *
from flask import Flask, render_template, request, redirect

app = Flask(__name__)

cars = {}
fobs = {}

factory = Factory()

@app.route('/')
def index():
    return render_template('index.html', car=str(cars), fob=str(fobs)) 

@app.route('/debug/add_paired', methods=['POST'])
def addPaired():
    if request.method == 'POST':
        key = request.form['key']
        if (not key in cars.keys() and not key in fobs.keys()):
            cars[key], fobs[key] = factory.gen_car_fob(random.randint(1, 0xffff))
    return redirect('/')
@app.route('/debug/add_unpaired', methods=['POST'])
def addUnpaired():
    if request.method == 'POST':
        print(request.form.keys())
        key = request.form['key']
        if (not key in cars.keys() and not key in fobs.keys()):
            fobs[key] = factory.gen_unpaired_fob()
    return redirect('/')

@app.route('/debug/reset', methods=['POST'])
def reset():
    car_fobs = []
    unpaired_fobs = []
if __name__ == '__main__':
    app.run(debug=True)
